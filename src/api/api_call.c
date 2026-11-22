#include "api_internal.h"
#include "../compiler/compiler.h"
#include "../parser/parser.h"
#include "../platform/platform.h"
#include "../modules/modules_internal.h"

/* ============================================================
 * Ejecución protegida
 * ============================================================ */

LunaStatus luna_call(LunaState* L, int nargs, int nresults) {
    return luna_pcall(L, nargs, nresults, 0);
}

LunaStatus luna_pcall(LunaState* L, int nargs, int nresults, int errfunc) {
    (void)errfunc;  /* TODO: implementar función de error */

    if (L->stack_top < (size_t)(nargs + 1)) {
        luna_error_set_simple(L, LUNA_ERR_ARGUMENT, "not enough values on stack");
        return L->error.status;
    }

    int func_idx = (int)L->stack_top - nargs - 1;

    /* Guardar estado del stack antes de la llamada */
    size_t saved_top = L->stack_top;
    int saved_status = L->error.status;

    /* Llamar a la función */
    int r = vm_call_function(L, func_idx, nargs, nresults < 0 ? -1 : nresults);

    if (r != 0 || L->error.status != LUNA_OK) {
        /* Error: restaurar stack al estado anterior, dejando el mensaje de error */
        /* En una implementación completa, haríamos un unwind del stack.
         * Por ahora, dejamos el error en L->error. */
        (void)saved_top;
        (void)saved_status;
        return L->error.status;
    }

    /* Ajustar resultados si nresults es específico */
    if (nresults >= 0) {
        /* vm_call_function ya ajustó el stack; verificar */
    }

    return LUNA_OK;
}

/* ============================================================
 * Carga y ejecución de archivos/strings
 * ============================================================ */

LunaStatus luna_dostring(LunaState* L, const char* src, const char* chunkname) {
    if (!src) {
        luna_error_set_simple(L, LUNA_ERR_ARGUMENT, "null source");
        return LUNA_ERR_ARGUMENT;
    }

    /* Parsear */
    LunaParser P;
    LunaNode* prog = luna_parse(src, strlen(src), &P);
    if (!prog) {
        luna_error_set(L, LUNA_ERR_SYNTAX,
                       chunkname ? chunkname : "<string>",
                       P.error_line, P.error_column,
                       "%s", luna_parser_error(&P) ? luna_parser_error(&P) : "parse error");
        return LUNA_ERR_SYNTAX;
    }

    /* Compilar */
    LunaProto* proto = luna_compile(prog);
    luna_ast_free(prog);
    if (!proto) {
        luna_error_set_simple(L, LUNA_ERR_RUNTIME, "compilation failed");
        return LUNA_ERR_RUNTIME;
    }

    /* Ejecutar */
    int r = luna_vm_execute(L, proto);
    luna_proto_free(proto);

    if (r != 0) {
        return L->error.status ? L->error.status : LUNA_ERR_RUNTIME;
    }

    return LUNA_OK;
}

LunaStatus luna_dofile(LunaState* L, const char* path) {
    if (!path) {
        luna_error_set_simple(L, LUNA_ERR_ARGUMENT, "null path");
        return LUNA_ERR_ARGUMENT;
    }

    modules_set_current_dir(&L->modules, path);

    char errbuf[256] = {0};
    size_t src_len = 0;
    char* src = luna_platform_read_file(path, &src_len, errbuf, sizeof(errbuf));
    if (!src) {
        luna_error_set(L, LUNA_ERR_IO, path, 0, 0, "%s", errbuf);
        return LUNA_ERR_IO;
    }

    LunaStatus status = luna_dostring(L, src, path);
    free(src);
    return status;
}