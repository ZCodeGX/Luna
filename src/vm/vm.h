#ifndef LUNA_VM_H
#define LUNA_VM_H

#include <stddef.h>

struct LunaState;
struct LunaProto;
struct LunaTable;

/* ============================================================
 * API pública de la VM
 * ============================================================ */

/*
 * Ejecuta un proto como chunk principal (entry point).
 * El proto se ejecuta como si fuera una función anónima sin parámetros.
 *
 * Devuelve 0 en éxito, -1 en error.
 * En caso de error, el mensaje queda en L->error (accesible vía luna_get_error_message).
 */
int luna_vm_execute(struct LunaState* L, struct LunaProto* proto);

/*
 * Ejecuta un proto como módulo: se le pasa una tabla de exports
 * donde la función debe escribir sus símbolos `pub`.
 *
 * Devuelve 0 en éxito, -1 en error.
 */
int luna_vm_execute_module(struct LunaState* L, struct LunaProto* proto,
                           struct LunaTable* exports);

/*
 * Inicializa el subsistema de VM en un estado.
 * Debe llamarse una vez tras crear el estado.
 */
void luna_vm_init(struct LunaState* L);

/*
 * Libera el subsistema de VM.
 */
void luna_vm_shutdown(struct LunaState* L);

#endif /* LUNA_VM_H */