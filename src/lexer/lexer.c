#include "lexer.h"
#include "lexer_internal.h"
#include "indent.h"
#include "number.h"
#include <stdio.h>
#include <string.h>

static void fill_error_token(LunaLexer* L, LunaToken* out) {
    fill_tok(out, LUNA_TOK_ERROR, L->error_message,
             strlen(L->error_message), L->error_line, L->error_column);
}

/* ---------- clasificación de identificadores ---------- */
static bool is_ident_start(char c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_'; }
static bool is_ident_cont(char c)  { return is_ident_start(c) || (c >= '0' && c <= '9'); }

/* ---------- palabras clave ---------- */
typedef struct { const char* name; size_t len; LunaTokenType type; } Kw;

static const Kw kKeywords[] = {
    {"let",      3, LUNA_TOK_LET},
    {"const",    5, LUNA_TOK_CONST},
    {"fn",       2, LUNA_TOK_FN},
    {"return",   6, LUNA_TOK_RETURN},
    {"if",       2, LUNA_TOK_IF},
    {"else",     4, LUNA_TOK_ELSE},
    {"for",      3, LUNA_TOK_FOR},
    {"in",       2, LUNA_TOK_IN},
    {"while",    5, LUNA_TOK_WHILE},
    {"break",    5, LUNA_TOK_BREAK},
    {"continue", 8, LUNA_TOK_CONTINUE},
    {"import",   6, LUNA_TOK_IMPORT},
    {"mod",      3, LUNA_TOK_MOD},
    {"pub",      3, LUNA_TOK_PUB},
    {"true",     4, LUNA_TOK_TRUE},
    {"false",    5, LUNA_TOK_FALSE},
    {"nil",      3, LUNA_TOK_NIL},
    {"and",      3, LUNA_TOK_AND},
    {"or",       2, LUNA_TOK_OR},
    {"not",      3, LUNA_TOK_NOT},
    {"try",      3, LUNA_TOK_TRY},
    {"catch",    5, LUNA_TOK_CATCH},
    {"throw",    5, LUNA_TOK_THROW},
};

static void lex_ident(LunaLexer* L, LunaToken* out) {
    int    line = L->line;
    int    col  = L->column;
    const char* start = L->source + L->pos;

    while (L->pos < L->length && is_ident_cont(L->source[L->pos])) adv(L);
    size_t n = (size_t)((L->source + L->pos) - start);

    LunaTokenType t = LUNA_TOK_IDENT;
    for (size_t i = 0; i < sizeof(kKeywords)/sizeof(kKeywords[0]); i++) {
        if (kKeywords[i].len == n && memcmp(start, kKeywords[i].name, n) == 0) {
            t = kKeywords[i].type;
            break;
        }
    }
    fill_tok(out, t, start, n, line, col);
}

/* ---------- cadenas ---------- */
static void lex_string(LunaLexer* L, LunaToken* out) {
    int    line = L->line;
    int    col  = L->column;
    adv(L); /* abre comilla */

    const char* start = L->source + L->pos;

    while (L->pos < L->length) {
        char c = L->source[L->pos];
        if (c == '"') {
            size_t n = (size_t)((L->source + L->pos) - start);
            adv(L);
            fill_tok(out, LUNA_TOK_STRING, start, n, line, col);
            return;
        }
        if (c == '\\') {
            adv(L);
            if (L->pos < L->length) adv(L);
            continue;
        }
        if (c == '\n' || c == '\r') {
            set_error(L, "unterminated string");
            return;
        }
        adv(L);
    }
    set_error(L, "unterminated string");
}

/* ---------- bucle principal ---------- */
bool luna_lexer_next(LunaLexer* L, LunaToken* out) {
    memset(out, 0, sizeof(*out));

    for (;;) {
        if (L->error) {
            fill_error_token(L, out);
            return false;
        }

        /* 1. DEDENTs pendientes */
        if (L->pending_dedents > 0) {
            L->pending_dedents--;
            fill_tok_empty(out, LUNA_TOK_DEDENT, L->line, 1);
            return true;
        }
        /* 2. INDENT pendiente */
        if (L->pending_indent) {
            L->pending_indent = false;
            fill_tok_empty(out, LUNA_TOK_INDENT, L->line, 1);
            return true;
        }
        /* 3. EOF real */
        if (L->pos >= L->length) {
            if (L->indent_depth > 1) {
                L->indent_depth--;
                fill_tok_empty(out, LUNA_TOK_DEDENT, L->line, 1);
                return true;
            }
            fill_tok_empty(out, LUNA_TOK_EOF, L->line, L->column);
            return false;
        }

        /* 4. Indentación (delegado a indent.c) */
        LunaIndentResult ir = luna_indent_handle(L);   /* sin 'out' */
        if (ir == LUNA_INDENT_EMITS)    return true;
        if (ir == LUNA_INDENT_CONTINUE) continue;
        /* LUNA_INDENT_NONE: seguimos */
        if (L->error) {
            fill_error_token(L, out);
            return false;
        }

        /* 5. Espacios/tabs intra-línea y comentarios */
        while (L->pos < L->length) {
            char c = L->source[L->pos];
            if (c == ' ' || c == '\t') { adv(L); continue; }
            if (c == '#') {
                while (L->pos < L->length && L->source[L->pos] != '\n'
                       && L->source[L->pos] != '\r') adv(L);
                break;
            }
            break;
        }
        if (L->pos >= L->length) continue;

        char c = L->source[L->pos];

        /* 6. Fin de línea */
        if (c == '\n' || c == '\r') {
            bool crlf = (c == '\r' && L->pos + 1 < L->length && L->source[L->pos + 1] == '\n');
            if (L->paren_depth > 0) {
                adv(L); if (crlf) adv(L);
                continue;
            }
            int line = L->line, col = L->column;
            adv(L); if (crlf) adv(L);
            L->at_line_start = true;
            fill_tok_empty(out, LUNA_TOK_NEWLINE, line, col);
            return true;
        }

        /* 7. Identificadores y keywords */
        if (is_ident_start(c)) { lex_ident(L, out); return true; }
        /* 8. Números (delegado a number.c) */
        if (c >= '0' && c <= '9') {
            luna_lex_number(L, out);
            if (L->error) { fill_error_token(L, out); return false; }
            return true;
        }
        /* 9. Cadenas */
        if (c == '"') {
            lex_string(L, out);
            if (L->error) { fill_error_token(L, out); return false; }
            return true;
        }

        /* 10. Operadores y puntuación */
        {
            int line = L->line, col = L->column;
            char c2 = (L->pos + 1 < L->length) ? L->source[L->pos + 1] : 0;
            LunaTokenType t = LUNA_TOK_ERROR;
            int n = 1;

            if      (c == '*' && c2 == '*') { t = LUNA_TOK_STAR_STAR;    n = 2; }
            else if (c == '=' && c2 == '=') { t = LUNA_TOK_EQ;           n = 2; }
            else if (c == '!' && c2 == '=') { t = LUNA_TOK_NEQ;          n = 2; }
            else if (c == '<' && c2 == '=') { t = LUNA_TOK_LE;           n = 2; }
            else if (c == '>' && c2 == '=') { t = LUNA_TOK_GE;           n = 2; }
            else if (c == '+' && c2 == '=') { t = LUNA_TOK_PLUS_ASSIGN;  n = 2; }
            else if (c == '-' && c2 == '=') { t = LUNA_TOK_MINUS_ASSIGN; n = 2; }
            else if (c == '*' && c2 == '=') { t = LUNA_TOK_STAR_ASSIGN;  n = 2; }
            else if (c == '/' && c2 == '=') { t = LUNA_TOK_SLASH_ASSIGN; n = 2; }
            else {
                switch (c) {
                    case '+': t = LUNA_TOK_PLUS;     break;
                    case '-': t = LUNA_TOK_MINUS;    break;
                    case '*': t = LUNA_TOK_STAR;     break;
                    case '/': t = LUNA_TOK_SLASH;    break;
                    case '%': t = LUNA_TOK_PERCENT;  break;
                    case '=': t = LUNA_TOK_ASSIGN;   break;
                    case '<': t = LUNA_TOK_LT;       break;
                    case '>': t = LUNA_TOK_GT;       break;
                    case '(': t = LUNA_TOK_LPAREN;   L->paren_depth++; break;
                    case ')': t = LUNA_TOK_RPAREN;   if (L->paren_depth > 0) L->paren_depth--; break;
                    case '[': t = LUNA_TOK_LBRACKET; L->paren_depth++; break;
                    case ']': t = LUNA_TOK_RBRACKET; if (L->paren_depth > 0) L->paren_depth--; break;
                    case '{': t = LUNA_TOK_LBRACE;   L->paren_depth++; break;
                    case '}': t = LUNA_TOK_RBRACE;   if (L->paren_depth > 0) L->paren_depth--; break;
                    case ',': t = LUNA_TOK_COMMA;    break;
                    case ';': t = LUNA_TOK_NEWLINE;  break;
                    case '.': t = LUNA_TOK_DOT;      break;
                    case ':': t = LUNA_TOK_COLON;    break;
                    default:
                        set_error_char(L, c);
                        continue;
                }
            }

            const char* start = L->source + L->pos;
            for (int i = 0; i < n; i++) adv(L);
            fill_tok(out, t, start, (size_t)n, line, col);
            return true;
        }
    }
}

/* ---------- API de control ---------- */
void luna_lexer_init(LunaLexer* L, const char* source, size_t length) {
    memset(L, 0, sizeof(*L));
    L->source           = source;
    L->length           = length;
    L->pos              = 0;
    L->line             = 1;
    L->column           = 1;
    L->indent_stack[0]  = 0;
    L->indent_depth     = 1;
    L->pending_dedents  = 0;
    L->pending_indent   = false;
    L->paren_depth      = 0;
    L->at_line_start    = true;
    L->error            = false;
}

const char* luna_lexer_error(const LunaLexer* L) {
    return L->error ? L->error_message : NULL;
}

void luna_lexer_free(LunaLexer* L) {
    (void)L;
}

/* ---------- nombres de token ---------- */
const char* luna_token_name(LunaTokenType t) {
    switch (t) {
        case LUNA_TOK_EOF:            return "EOF";
        case LUNA_TOK_ERROR:          return "ERROR";
        case LUNA_TOK_NEWLINE:        return "NEWLINE";
        case LUNA_TOK_INDENT:         return "INDENT";
        case LUNA_TOK_DEDENT:         return "DEDENT";
        case LUNA_TOK_INTEGER:        return "INTEGER";
        case LUNA_TOK_FLOAT:          return "FLOAT";
        case LUNA_TOK_STRING:         return "STRING";
        case LUNA_TOK_IDENT:          return "IDENT";
        case LUNA_TOK_LET:            return "LET";
        case LUNA_TOK_CONST:          return "CONST";
        case LUNA_TOK_FN:             return "FN";
        case LUNA_TOK_RETURN:         return "RETURN";
        case LUNA_TOK_IF:             return "IF";
        case LUNA_TOK_ELSE:           return "ELSE";
        case LUNA_TOK_FOR:            return "FOR";
        case LUNA_TOK_IN:             return "IN";
        case LUNA_TOK_WHILE:          return "WHILE";
        case LUNA_TOK_BREAK:          return "BREAK";
        case LUNA_TOK_CONTINUE:       return "CONTINUE";
        case LUNA_TOK_IMPORT:         return "IMPORT";
        case LUNA_TOK_MOD:            return "MOD";
        case LUNA_TOK_PUB:            return "PUB";
        case LUNA_TOK_TRUE:           return "TRUE";
        case LUNA_TOK_FALSE:          return "FALSE";
        case LUNA_TOK_NIL:            return "NIL";
        case LUNA_TOK_AND:            return "AND";
        case LUNA_TOK_OR:             return "OR";
        case LUNA_TOK_NOT:            return "NOT";
        case LUNA_TOK_TRY:            return "TRY";
        case LUNA_TOK_CATCH:          return "CATCH";
        case LUNA_TOK_THROW:          return "THROW";
        case LUNA_TOK_PLUS:           return "PLUS";
        case LUNA_TOK_MINUS:          return "MINUS";
        case LUNA_TOK_STAR:           return "STAR";
        case LUNA_TOK_SLASH:          return "SLASH";
        case LUNA_TOK_PERCENT:        return "PERCENT";
        case LUNA_TOK_STAR_STAR:      return "STAR_STAR";
        case LUNA_TOK_EQ:             return "EQ";
        case LUNA_TOK_NEQ:            return "NEQ";
        case LUNA_TOK_LT:             return "LT";
        case LUNA_TOK_GT:             return "GT";
        case LUNA_TOK_LE:             return "LE";
        case LUNA_TOK_GE:             return "GE";
        case LUNA_TOK_ASSIGN:         return "ASSIGN";
        case LUNA_TOK_PLUS_ASSIGN:    return "PLUS_ASSIGN";
        case LUNA_TOK_MINUS_ASSIGN:   return "MINUS_ASSIGN";
        case LUNA_TOK_STAR_ASSIGN:    return "STAR_ASSIGN";
        case LUNA_TOK_SLASH_ASSIGN:   return "SLASH_ASSIGN";
        case LUNA_TOK_LPAREN:         return "LPAREN";
        case LUNA_TOK_RPAREN:         return "RPAREN";
        case LUNA_TOK_LBRACKET:       return "LBRACKET";
        case LUNA_TOK_RBRACKET:       return "RBRACKET";
        case LUNA_TOK_LBRACE:         return "LBRACE";
        case LUNA_TOK_RBRACE:         return "RBRACE";
        case LUNA_TOK_COMMA:          return "COMMA";
        case LUNA_TOK_DOT:            return "DOT";
        case LUNA_TOK_COLON:          return "COLON";
    }
    return "UNKNOWN";
}