#ifndef LUNA_STDLIB_INTERNAL_H
#define LUNA_STDLIB_INTERNAL_H

#include "../api/api_internal.h"
#include "luna_stdlib.h"

typedef struct LunaTable LunaTable;

int lib_nargs(LunaState* L);
LunaValue lib_arg(LunaState* L, int i);
int lib_error(LunaState* L, const char* fmt, ...);
char* lib_strdup(const char* s);
void lib_push_value(LunaState* L, LunaValue value);
int lib_builtin_len(LunaState* L);
LunaValue lib_value_from_string(LunaState* L, const char* s, size_t len);
void lib_export_function(LunaState* L, LunaTable* exports, const char* name,
                         int (*fn)(LunaState* L));
void lib_export_float(LunaState* L, LunaTable* exports, const char* name, double value);
void lib_export_string(LunaState* L, LunaTable* exports, const char* name,
                       const char* value);

void luna_open_math_lib(LunaState* L);
void luna_open_string_lib(LunaState* L);
void luna_open_collection_lib(LunaState* L);
void luna_open_system_lib(LunaState* L);

#endif
