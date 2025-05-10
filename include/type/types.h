#pragma once

#include "errors/error.h"       // error
#include "intf/mm.h"            // lalloc
#include "utils/deserializer.h" //  deserializer
#include "utils/serializer.h"   //  serializer

#include "type/types/enum.h"   // enum_t
#include "type/types/prim.h"   // prim_t
#include "type/types/sarray.h" // sarray_t
#include "type/types/struct.h" // struct_t
#include "type/types/union.h"  // union_t

/**
 * TYPE = PRIM | STRUCT | SARRAY | UNION | ENUM
 * STRUCT = KEY TYPE (, KEY TYPE)*
 * SARRAY = DIM (, DIM)* TYPE
 * UNION = KEY TYPE (, KEY TYPE)*
 * ENUM = KEY (, KEY)*
 */

typedef struct type_s type;

typedef enum
{
  T_PRIM,
  T_STRUCT,
  T_UNION,
  T_ENUM,
  T_VARRAY,
  T_SARRAY,
} type_t;

struct type_s
{
  union
  {
    prim_t p;
    struct_t st;
    union_t un;
    enum_t en;
    sarray_t sa;
  };

  type_t type;
};

/**
 * Checks that this type is valid
 */
err_t type_validate (const type *t, error *e);

/**
 * Cleanly print to string
 */
int type_snprintf (char *str, u32 size, type *t);

/**
 * Get the size in bytes
 */
u32 type_byte_size (const type *t);

/**
 * Frees internals of a possibly invalid type
 */
void type_free_internals_forgiving (type *t, lalloc *alloc);

/**
 * Frees internals of a valid type
 */
void type_free_internals (type *t, lalloc *alloc);

/**
 * Get the size of the buffer needed to serialize this type
 */
u32 type_get_serial_size (const type *t);

/**
 * Serialize src into dest - assumes dest is long enough
 * Call get_serial_size first
 */
void type_serialize (serializer *dest, const type *src);

/**
 * Used by user
 */
err_t type_deserialize (type *dest, deserializer *src, lalloc *alloc, error *e);
