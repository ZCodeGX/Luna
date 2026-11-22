#include "error.h"

const char* luna_status_name(LunaStatus status) {
    switch (status) {
        case LUNA_OK:           return "OK";
        case LUNA_ERR_SYNTAX:   return "syntax error";
        case LUNA_ERR_RUNTIME:  return "runtime error";
        case LUNA_ERR_TYPE:     return "type error";
        case LUNA_ERR_MEMORY:   return "memory error";
        case LUNA_ERR_MODULE:   return "module error";
        case LUNA_ERR_NATIVE:   return "native error";
        case LUNA_ERR_ARGUMENT: return "argument error";
        case LUNA_ERR_IO:       return "I/O error";
        case LUNA_ERR_LIMIT:    return "limit error";
        case LUNA_ERR_DIVZERO:  return "division by zero";
        case LUNA_ERR_INDEX:    return "index out of range";
        case LUNA_ERR_KEY:      return "key not found";
        case LUNA_ERR_ASSERT:   return "assertion failed";
        case LUNA_ERR_USER:     return "user error";
    }
    return "unknown error";
}
