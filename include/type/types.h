#pragma once

#include "dev/errors.h"
#include "intf/mm.h"

#include "type/types/enum.h"
#include "type/types/prim.h"
#include "type/types/sarray.h"
#include "type/types/struct.h"
#include "type/types/union.h"
#include "type/types/varray.h"

/**
 * TYPE = PRIM | STRUCT | VARRAY | SARRAY | UNION | ENUM
 * STRUCT = KEY TYPE (, KEY TYPE)*
 * VARRAY = RANK TYPE
 * SARRAY = DIM (, DIM)* TYPE
 * UNION = KEY TYPE (, KEY TYPE)*
 * ENUM = KEY (, KEY)*
 */

typedef struct type_s type;

struct type_s
{
  union
  {
    prim_t p;
    struct_t st;
    union_t un;
    enum_t en;
    varray_t va;
    sarray_t sa;
  };

  type_t type;
};

// Logging
void i_log_type (const type *t);

// Size in bits
err_t type_bits_size (u64 *dest, const type *t);

// Free
void type_free_internals (type t, lalloc *alloc);

// Serialization
err_t type_get_serial_size (u16 *dest, const type *t);
err_t type_serialize (u8 *dest, u16 dlen, const type *src);
err_t type_deserialize (type *dest, lalloc *a, const u8 *src, u16 slen);
