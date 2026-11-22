#include "error.h"
#include "errors_internal.h"
#include "../runtime/runtime_internal.h"
#include <string.h>

/* ============================================================
 * Traceback
 * ============================================================ */

void luna_traceback_push(struct LunaState* L,
                         const char* function_name,
                         const char* source_file,
                         int line, int pc) {
    LunaError* err = &L->error;
    LunaTraceback* tb = &err->traceback;

    if (tb->count >= LUNA_MAX_TRACEBACK_FRAMES) {
        /* Traceback lleno: no añadir más frames.
         * En el futuro podríamos guardar un flag "truncated". */
        return;
    }

    LunaTracebackFrame* f = &tb->frames[tb->count++];
    f->function_name = function_name;  /* el caller garantiza que sobrevive */
    f->source_file = source_file;
    f->line = line;
    f->pc = pc;
}

void luna_traceback_clear(struct LunaState* L) {
    L->error.traceback.count = 0;
}
