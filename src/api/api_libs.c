#include "api_internal.h"

static void print_value(LunaValue v) {
    switch (v.type) {
        case LUNA_TNIL:
            fputs("nil", stdout);
            break;
        case LUNA_TBOOLEAN:
            fputs(luna_val_bool(v) ? "true" : "false", stdout);
            break;
        case LUNA_TINTEGER:
            printf("%lld", (long long)luna_val_int(v));
            break;
        case LUNA_TFLOAT:
            printf("%g", luna_val_float(v));
            break;
        case LUNA_TSTRING: {
            LunaString* s = luna_val_string(v);
            fwrite(s->data, 1, s->len, stdout);
            break;
        }
        default:
            fputs(luna_type_name(v.type), stdout);
            break;
    }
}

static int builtin_print(LunaState* L) {
    LunaVM* vm = state_vm(L);
    CallFrame* frame = &vm->frames[vm->frame_top - 1];
    int nargs = frame->num_args;

    for (int i = 0; i < nargs; i++) {
        if (i) putchar(' ');
        print_value(frame->base[1 + (size_t)i]);
    }
    putchar('\n');
    return 0;
}

void luna_open_libs(LunaState* L) {
    luna_pushcfunction(L, builtin_print);
    luna_setglobal(L, "print");
}
