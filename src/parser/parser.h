#ifndef LUNA_PARSER_H
#define LUNA_PARSER_H

#include "../ast/ast.h"
#include "../lexer/lexer.h"
#include "../../include/luna/diagnostics.h"
#include <stdbool.h>
#include <stddef.h>

typedef struct {
    LunaLexer lexer;
    LunaToken current;
    LunaToken previous;
    bool      has_current;
    bool      has_previous;
    bool      failed;
    int       error_count;
    int       expr_depth;
    int       diag_start;
    char      error_message[256];
    int       error_line;
    int       error_column;

    const char* source;
    size_t      source_len;
    const char* filename;
    LunaDiagnosticSink* diags;
    LunaDiagnosticSink owned_diags;
    bool owns_diags;
    bool panic_mode;
    int loop_depth;
} LunaParser;

LunaNode* luna_parse(const char* source, size_t length, LunaParser* P_out);
LunaNode* luna_parse_named(const char* source, size_t length,
                           const char* filename, LunaParser* P_out);
LunaNode* luna_parse_with_diagnostics(const char* source, size_t length,
                                      const char* filename,
                                      LunaDiagnosticSink* sink, LunaParser* P_out);
const char* luna_parser_error(const LunaParser* P);
int luna_parser_error_count(const LunaParser* P);
int luna_parser_diagnostic_count(const LunaParser* P);
void luna_parser_render_diagnostics(const LunaParser* P, void* out_stream, bool use_color);

void luna_parser_init(LunaParser* P, const char* source, size_t length);
void luna_parser_init_named(LunaParser* P, const char* source, size_t length,
                            const char* filename, LunaDiagnosticSink* sink);
void luna_parser_destroy(LunaParser* P);
LunaNode* luna_parser_parse_program(LunaParser* P);

#endif
