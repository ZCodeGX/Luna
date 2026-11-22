#include "parser_internal.h"

#include <stdlib.h>
#include <string.h>

void luna_parser_init_named(LunaParser* P, const char* source, size_t length,
                            const char* filename, LunaDiagnosticSink* sink) {
    memset(P, 0, sizeof(*P));
    P->source = source;
    P->source_len = length;
    P->filename = filename ? filename : "<string>";

    if (sink) {
        P->diags = sink;
        P->owns_diags = false;
    } else {
        luna_diag_sink_init(&P->owned_diags, luna_diag_max_errors());
        P->diags = &P->owned_diags;
        P->owns_diags = true;
    }

    luna_lexer_init(&P->lexer, source, length);
    advance(P);
}

void luna_parser_init(LunaParser* P, const char* source, size_t length) {
    luna_parser_init_named(P, source, length, "<string>", NULL);
}

void luna_parser_destroy(LunaParser* P) {
    if (P->owns_diags) luna_diag_sink_free(&P->owned_diags);
    P->diags = NULL;
    P->owns_diags = false;
}

static bool token_changed(const LunaParser* P, LunaTokenType type,
                          const char* start, size_t length) {
    return P->current.type != type || P->current.start != start ||
           P->current.length != length;
}

LunaNode* luna_parser_parse_program(LunaParser* P) {
    while (check(P, LUNA_TOK_NEWLINE)) advance(P);

    LunaNodeList* head = NULL;
    LunaNodeList* tail = NULL;

    for (;;) {
        if (check(P, LUNA_TOK_EOF)) break;

        if (check(P, LUNA_TOK_ERROR)) {
            fail_current(P, "error léxico");
            advance(P);
            continue;
        }

        if (check(P, LUNA_TOK_INDENT)) {
            fail_current(P, "unexpected indentation at file start");
            synchronize(P);
            continue;
        }

        if (P->panic_mode) synchronize(P);
        if (check(P, LUNA_TOK_EOF)) break;

        LunaTokenType before_type = P->current.type;
        const char* before_start = P->current.start;
        size_t before_length = P->current.length;

        LunaNode* stmt = parse_statement(P);
        if (stmt) {
            LunaNodeList* item = (LunaNodeList*)calloc(1, sizeof(LunaNodeList));
            item->node = stmt;
            if (!head) head = item; else tail->next = item;
            tail = item;
        }

        if (P->panic_mode) synchronize(P);
        if (!token_changed(P, before_type, before_start, before_length) &&
            !check(P, LUNA_TOK_EOF)) {
            advance(P);
        }

        while (check(P, LUNA_TOK_NEWLINE)) advance(P);
    }

    if (luna_diag_has_errors(P->diags)) {
        luna_ast_free(luna_node_program(head, 1, 1));
        return NULL;
    }
    return luna_node_program(head, 1, 1);
}

LunaNode* luna_parse_with_diagnostics(const char* source, size_t length,
                                      const char* filename,
                                      LunaDiagnosticSink* sink, LunaParser* P_out) {
    luna_parser_init_named(P_out, source, length, filename, sink);
    return luna_parser_parse_program(P_out);
}

LunaNode* luna_parse_named(const char* source, size_t length,
                           const char* filename, LunaParser* P_out) {
    return luna_parse_with_diagnostics(source, length, filename, NULL, P_out);
}

LunaNode* luna_parse(const char* source, size_t length, LunaParser* P_out) {
    return luna_parse_named(source, length, "<string>", P_out);
}

const char* luna_parser_error(const LunaParser* P) {
    if (P->error_message[0]) return P->error_message;
    return luna_diag_has_errors(P->diags) ? "parse error" : NULL;
}

int luna_parser_error_count(const LunaParser* P) {
    return P->diags ? P->diags->error_count : 0;
}

int luna_parser_diagnostic_count(const LunaParser* P) {
    return P->diags ? P->diags->count : 0;
}

void luna_parser_render_diagnostics(const LunaParser* P, void* out_stream, bool use_color) {
    if (!P->diags) return;
    luna_diag_render_all(P->diags, P->source, P->source_len,
                         P->filename, out_stream, use_color);
}
