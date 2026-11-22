#ifndef LUNA_BYTECODE_VERSION_H
#define LUNA_BYTECODE_VERSION_H

/*
 * Formato de archivo .luac:
 *
 *   [0..3]   Magic: "LUNA"
 *   [4]      Major version
 *   [5]      Minor version
 *   [6..]    Proto raíz (formato recursivo)
 *
 * Reglas de compatibilidad:
 *   - Mismo major = compatible binario.
 *   - Minor distinto = puede leer, pero advierte.
 *   - Major distinto = error de lectura.
 */

#define LUNA_BYTECODE_MAGIC_0 'L'
#define LUNA_BYTECODE_MAGIC_1 'U'
#define LUNA_BYTECODE_MAGIC_2 'N'
#define LUNA_BYTECODE_MAGIC_3 'A'

#define LUNA_BYTECODE_VERSION_MAJOR 0
#define LUNA_BYTECODE_VERSION_MINOR 1

#endif /* LUNA_BYTECODE_VERSION_H */