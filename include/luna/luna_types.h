#ifndef LUNA_LUNA_TYPES_H
#define LUNA_LUNA_TYPES_H

#include <stddef.h>
#include <stdint.h>

typedef enum {
    LUNA_OK            = 0,
    LUNA_ERR_SYNTAX    = 1,
    LUNA_ERR_RUNTIME   = 2,
    LUNA_ERR_TYPE      = 3,
    LUNA_ERR_MEMORY    = 4,
    LUNA_ERR_MODULE    = 5,
    LUNA_ERR_NATIVE    = 6,
    LUNA_ERR_ARGUMENT  = 7,
    LUNA_ERR_IO        = 8,
    LUNA_ERR_LIMIT     = 9,
    LUNA_ERR_DIVZERO   = 10,
    LUNA_ERR_INDEX     = 11,
    LUNA_ERR_KEY       = 12,
    LUNA_ERR_ASSERT    = 13,
    LUNA_ERR_USER      = 14
} LunaStatus;

typedef enum {
    LUNA_TNIL      = 0,
    LUNA_TBOOLEAN  = 1,
    LUNA_TINTEGER  = 2,
    LUNA_TFLOAT    = 3,
    LUNA_TSTRING   = 4,
    LUNA_TLIST     = 5,
    LUNA_TTABLE    = 6,
    LUNA_TFUNCTION = 7,
    LUNA_TUSERDATA = 8
} LunaType;

typedef struct LunaState LunaState;

typedef void* (*LunaAllocFn)(void* ud, void* ptr, size_t old_size, size_t new_size);

typedef struct LunaAllocator {
    void*       ud;
    LunaAllocFn alloc;
} LunaAllocator;

typedef int  (*LunaCFunction)(LunaState* L);
typedef void (*LunaFinalizer)(LunaState* L, void* ud);

#endif
