#ifndef LUNA_SCOPE_H
#define LUNA_SCOPE_H

#include <stdint.h>
#include <stdbool.h>

#define LUNA_MAX_LOCALS 255
#define LUNA_MAX_UPVALUES 255

typedef struct {
    char* name;
    uint8_t slot;
    bool is_const;
} LunaLocal;

typedef struct {
    bool is_local;
    uint8_t index;
} LunaUpvalue;

typedef struct {
    LunaLocal locals[LUNA_MAX_LOCALS];
    int num_locals;

    LunaUpvalue upvalues[LUNA_MAX_UPVALUES];
    int num_upvalues;

    int depth;
    int scope_start;
} LunaScope;

void scope_init(LunaScope* S, int depth);
int scope_add_local(LunaScope* S, const char* name, bool is_const);
int scope_find_local(LunaScope* S, const char* name);
void scope_hide_local(LunaScope* S, int slot);
int scope_add_upvalue(LunaScope* S, bool is_local, uint8_t index);
void scope_free(LunaScope* S);

#endif