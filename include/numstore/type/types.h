#pragma once

#include "core/errors/error.h"       // error
#include "core/mm/lalloc.h"          // lalloc
#include "core/utils/deserializer.h" //  deserializer
#include "core/utils/serializer.h"   //  serializer

#include "numstore/type/types/enum.h"   // enum_t
#include "numstore/type/types/prim.h"   // prim_t
#include "numstore/type/types/sarray.h" // sarray_t
#include "numstore/type/types/struct.h" // struct_t
#include "numstore/type/types/union.h"  // union_t

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
  T_PRIM = 0,
  T_STRUCT = 1,
  T_UNION = 2,
  T_ENUM = 3,
  T_SARRAY = 4,
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
 * Otherwise returns error
 */
err_t type_validate (const type *t, error *e);

/**
 * Cleanly print to string
 */
i32 type_snprintf (char *str, u32 size, type *t);

/**
 * Get the size in bytes
 */
u32 type_byte_size (const type *t);

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

/**
 * Creates a random type
 *
 * Errors:
 *   - ERR_NOMEM - allocation exceeded
 */
err_t type_random (type *dest, lalloc *alloc, u32 depth, error *e);

/**
 * Checks if [left] == [right] deeply
 */
bool type_equal (const type *left, const type *right);

/**
 * Logs type - Allocates memory under the hood - no lalloc
 *
 *
 * Errors:
 *  - ERR_NOMEM
 */
err_t i_log_type (type t, error *e);
