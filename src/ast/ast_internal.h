#ifndef LUNA_AST_INTERNAL_H
#define LUNA_AST_INTERNAL_H

#include <stddef.h>

/* Helper interno para duplicar strings con longitud conocida */
char* luna_strdup_n(const char* s, size_t n);

#endif /* LUNA_AST_INTERNAL_H */