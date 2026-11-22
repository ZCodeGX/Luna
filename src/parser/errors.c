#include "parser_internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void fail_at(LunaParser* P, int line, int col, const char* msg) {
    if (P->failed) return;
    P->failed = true;
    snprintf(P->error_message, sizeof(P->error_message), "%s", msg);
    P->error_line = line;
    P->error_column = col;
}

void fail_current(LunaParser* P, const char* msg) {
    if (check(P, LUNA_TOK_ERROR) && P->current.start) {
        msg = (const char*)P->current.start;
    }
    fail_at(P, P->current.line, P->current.column, msg);
}

bool expect(LunaParser* P, LunaTokenType t, const char* what) {
    if (match(P, t)) return true;
    char msg[128];
    snprintf(msg, sizeof(msg), "expected %s", what);
    fail_current(P, msg);
    return false;
}

bool expect_block_end(LunaParser* P, const char* what) {
    match(P, LUNA_TOK_COLON);
    return expect(P, LUNA_TOK_NEWLINE, what);
}

char* copy_token_text(const LunaToken* tok) {
    if (!tok->start || tok->length == 0) {
        char* s = (char*)malloc(1);
        if (s) s[0] = 0;
        return s;
    }
    char* s = (char*)malloc(tok->length + 1);
    if (!s) return NULL;
    memcpy(s, tok->start, tok->length);
    s[tok->length] = 0;
    return s;
}