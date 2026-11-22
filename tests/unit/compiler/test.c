#include "parser.h"
#include "compiler.h"
#include "proto.h"          /* <-- cambia de "bytecode/proto.h" a "proto.h" */
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char** argv) {
    if (argc != 2) {
        fprintf(stderr, "usage: %s file.luna\n", argv[0]);
        return 2;
    }
    
    FILE* f = fopen(argv[1], "rb");
    if (!f) {
        perror(argv[1]);
        return 1;
    }
    
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    char* src = (char*)malloc((size_t)sz + 1);
    if (fread(src, 1, (size_t)sz, f) != (size_t)sz) {
        perror("read");
        free(src);
        fclose(f);
        return 1;
    }
    src[sz] = 0;
    fclose(f);
    
    /* Parsear */
    LunaParser P;
    LunaNode* prog = luna_parse(src, (size_t)sz, &P);
    if (!prog) {
        fprintf(stderr, "parse error: %d:%d: %s\n", 
                P.error_line, P.error_column,
                luna_parser_error(&P) ? luna_parser_error(&P) : "(no message)");
        free(src);
        return 1;
    }
    
    /* Compilar */
    LunaProto* proto = luna_compile(prog);
    if (!proto) {
        fprintf(stderr, "compile error\n");
        luna_ast_free(prog);
        free(src);
        return 1;
    }
    
    printf("Compilation successful!\n");
    printf("Function: %s\n", proto->name);
    printf("Instructions: %zu bytes\n", proto->code_len);
    printf("Constants: %zu\n", proto->num_constants);
    printf("Child protos: %zu\n", proto->num_protos);
    
    luna_proto_free(proto);
    luna_ast_free(prog);
    free(src);
    
    return 0;
}