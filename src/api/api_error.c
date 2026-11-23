#include "api_internal.h"

LunaStatus luna_status(LunaState* L) {
    return (LunaStatus)L->error.status;
}

const char* luna_last_error(LunaState* L) {
    return luna_get_error_message(L);
}

const char* luna_last_error_file(LunaState* L) {
    return luna_get_error_file(L);
}

int luna_last_error_line(LunaState* L) {
    return luna_get_error_line(L);
}

const char* luna_last_traceback(LunaState* L) {
    static char buf[2048];
    luna_format_error(L, buf, sizeof(buf));
    return buf;
}

void luna_clear_error(LunaState* L) {
    L->status = LUNA_OK;
    L->error.status = LUNA_OK;
    L->error.has_error = 0;
    L->error.message[0] = '\0';
    L->error.file[0] = '\0';
    L->error.line = 0;
    L->error.column = 0;
    L->error.traceback.count = 0;
}