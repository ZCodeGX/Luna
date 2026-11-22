#include "lexer.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char** argv) {
    if (argc != 2) { fprintf(stderr, "usage: %s file.luna\n", argv[0]); return 2; }

    FILE* f = fopen(argv[1], "rb");
    if (!f) { perror(argv[1]); return 1; }
    fseek(f, 0, SEEK_END); long sz = ftell(f); fseek(f, 0, SEEK_SET);
    char* src = malloc((size_t)sz + 1);
    if (fread(src, 1, (size_t)sz, f) != (size_t)sz) { perror("read"); return 1; }
    src[sz] = 0;
    fclose(f);

    LunaLexer L;
    luna_lexer_init(&L, src, (size_t)sz);

    LunaToken tok;
    while (luna_lexer_next(&L, &tok)) {
        if (tok.type == LUNA_TOK_INTEGER) {
            printf("%3d:%-3d INTEGER   %lld\n", tok.line, tok.column, (long long)tok.int_value);
        } else if (tok.type == LUNA_TOK_FLOAT) {
            printf("%3d:%-3d FLOAT     %g\n", tok.line, tok.column, tok.float_value);
        } else if (tok.type == LUNA_TOK_NEWLINE || tok.type == LUNA_TOK_INDENT || tok.type == LUNA_TOK_DEDENT) {
            printf("%3d:%-3d %s\n", tok.line, tok.column, luna_token_name(tok.type));
        } else {
            printf("%3d:%-3d %-9s %.*s\n",
                   tok.line, tok.column, luna_token_name(tok.type),
                   (int)tok.length, tok.start ? tok.start : "");
        }
    }
    if (L.error) {
        fprintf(stderr, "%d:%d: %s\n", L.error_line, L.error_column, L.error_message);
        free(src); return 1;
    }
    free(src);
    return 0;
}