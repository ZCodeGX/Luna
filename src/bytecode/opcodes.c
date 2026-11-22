#include "opcodes.h"

const char* luna_opcode_name(LunaOpcode op) {
    static const char* names[OP_COUNT] = {
        [OP_NOP]           = "NOP",
        [OP_LOAD_NIL]      = "LOAD_NIL",
        [OP_LOAD_TRUE]     = "LOAD_TRUE",
        [OP_LOAD_FALSE]    = "LOAD_FALSE",
        [OP_LOAD_CONST]    = "LOAD_CONST",
        [OP_LOAD_INT]      = "LOAD_INT",
        [OP_MOVE]          = "MOVE",
        [OP_GET_UPVALUE]   = "GET_UPVALUE",
        [OP_SET_UPVALUE]   = "SET_UPVALUE",
        [OP_GET_GLOBAL]    = "GET_GLOBAL",
        [OP_SET_GLOBAL]    = "SET_GLOBAL",
        [OP_GET_FIELD]     = "GET_FIELD",
        [OP_SET_FIELD]     = "SET_FIELD",
        [OP_GET_INDEX]     = "GET_INDEX",
        [OP_SET_INDEX]     = "SET_INDEX",
        [OP_NEW_TABLE]     = "NEW_TABLE",
        [OP_NEW_LIST]      = "NEW_LIST",
        [OP_LIST_APPEND]   = "LIST_APPEND",
        [OP_TABLE_SET]     = "TABLE_SET",
        [OP_ADD]           = "ADD",
        [OP_SUB]           = "SUB",
        [OP_MUL]           = "MUL",
        [OP_DIV]           = "DIV",
        [OP_MOD]           = "MOD",
        [OP_POW]           = "POW",
        [OP_NEG]           = "NEG",
        [OP_EQ]            = "EQ",
        [OP_NEQ]           = "NEQ",
        [OP_LT]            = "LT",
        [OP_LE]            = "LE",
        [OP_GT]            = "GT",
        [OP_GE]            = "GE",
        [OP_NOT]           = "NOT",
        [OP_AND]           = "AND",
        [OP_OR]            = "OR",
        [OP_CONCAT]        = "CONCAT",
        [OP_LEN]           = "LEN",
        [OP_JUMP]          = "JUMP",
        [OP_JUMP_IF_TRUE]  = "JUMP_IF_TRUE",
        [OP_JUMP_IF_FALSE] = "JUMP_IF_FALSE",
        [OP_CALL]          = "CALL",
        [OP_RETURN]        = "RETURN",
        [OP_CLOSURE]       = "CLOSURE",
        [OP_FOR_INIT]      = "FOR_INIT",
        [OP_FOR_STEP]      = "FOR_STEP",
        [OP_THROW]         = "THROW",
        [OP_SETUP_EXC]     = "SETUP_EXC",
        [OP_POP_EXC]       = "POP_EXC",
    };
    if (op < 0 || op >= OP_COUNT) return "?";
    return names[op] ? names[op] : "?";
}

LunaOpMode luna_opcode_mode(LunaOpcode op) {
    switch (op) {
        case OP_NOP:
        case OP_POP_EXC:
            return OP_ARG_NONE;

        case OP_LOAD_NIL:
        case OP_LOAD_TRUE:
        case OP_LOAD_FALSE:
        case OP_NEW_TABLE:
        case OP_NEW_LIST:
        case OP_THROW:
            return OP_ARG_A;

        case OP_MOVE:
        case OP_GET_UPVALUE:
        case OP_SET_UPVALUE:
        case OP_NEG:
        case OP_NOT:
        case OP_LEN:
        case OP_LIST_APPEND:
            return OP_ARG_AB;

        case OP_ADD: case OP_SUB: case OP_MUL: case OP_DIV:
        case OP_MOD: case OP_POW:
        case OP_EQ: case OP_NEQ: case OP_LT: case OP_LE:
        case OP_GT: case OP_GE:
        case OP_AND: case OP_OR:
        case OP_GET_FIELD: case OP_SET_FIELD:
        case OP_GET_INDEX: case OP_SET_INDEX:
        case OP_TABLE_SET:
        case OP_CONCAT:
        case OP_CALL:
        case OP_RETURN:
            return OP_ARG_ABC;

        case OP_LOAD_CONST:
        case OP_GET_GLOBAL:
        case OP_SET_GLOBAL:
        case OP_CLOSURE:
            return OP_ARG_ABx;

        case OP_LOAD_INT:
        case OP_JUMP_IF_TRUE:
        case OP_JUMP_IF_FALSE:
        case OP_FOR_INIT:
        case OP_FOR_STEP:
        case OP_SETUP_EXC:
            return OP_ARG_AsBx;

        case OP_JUMP:
            return OP_ARG_sBx;

        default:
            return OP_ARG_NONE;
    }
}