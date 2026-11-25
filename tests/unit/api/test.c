#include <stdio.h>
#include "luna/luna.h"

int main(void) {
    LunaState* L = luna_new(NULL);
    if (!L) return 1;

    const char* src =
        "boom = fn():\n"
        "    throw \"fallo\"\n"
        "\n"
        "handler = fn(msg):\n"
        "    return 42\n";

    if (luna_dostring(L, src, "=pcall_test") != LUNA_OK) {
        fprintf(stderr, "setup: %s\n", luna_last_error(L));
        luna_close(L);
        return 1;
    }

    luna_settop(L, 0);

    luna_getglobal(L, "handler");
    luna_getglobal(L, "boom");
    LunaStatus st = luna_pcall(L, 0, 1, 1);
    int ok = 0;
    int64_t value = luna_tointeger(L, -1, &ok);

    if (st == LUNA_OK || !ok || value != 42) {
        fprintf(stderr, "pcall errfunc falló: status=%d valor=%lld ok=%d\n",
                (int)st, (long long)value, ok);
        luna_close(L);
        return 1;
    }

    printf("pcall errfunc OK\n");
    luna_close(L);
    return 0;
}
