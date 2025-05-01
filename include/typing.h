#pragma once

#include "dev/assert.h"
#include "dev/errors.h"
#include "ds/strings.h"
#include "intf/mm.h"
#include "intf/types.h"

/**
 * TYPE = PRIM | STRUCT | VARRAY | SARRAY | UNION | ENUM
 * STRUCT = KEY TYPE (, KEY TYPE)*
 * VARRAY = RANK TYPE
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

typedef enum
{
  U8 = 0,
  U16 = 1,
  U32 = 2,
  U64 = 3,

  I8 = 4,
  I16 = 5,
  I32 = 6,
  I64 = 7,

  F16 = 8,
  F32 = 9,
  F64 = 10,
  F128 = 11,

  CF32 = 12,
  CF64 = 13,
  CF128 = 14,
  CF256 = 15,

  CI16 = 16,
  CI32 = 17,
  CI64 = 18,
  CI128 = 19,

  CU16 = 20,
  CU32 = 21,
  CU64 = 22,
  CU128 = 23,

  BOOL = 24,
  BIT = 25,
} prim_t;

typedef struct
{
  u16 len;
  string *keys;
  type *types;
} struct_t;

typedef struct
{
  u16 len;
  string *keys;
  type *types;
} union_t;

typedef struct
{
  u16 len;
  string *keys;
} enum_t;

typedef struct
{
  u16 rank;
  type *t; // Not an array
} varray_t;

typedef struct
{
  u16 rank;
  u32 *dims;
  type *t; // Not an array
} sarray_t;

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

void i_log_type (const type *t);
err_t type_get_serial_size (u16 *dest, const type *t);
err_t type_bits_size (u64 *dest, const type *t);
void type_free_internals (type *t, lalloc *alloc);

typedef struct
{
  type *src;
  u8 *dest;
  u16 dlen;
} type_serialize_params;

typedef struct
{
  type *dest;
  lalloc *type_allocator;
  u8 *src;
  u16 dlen;
} type_deserialize_params;

err_t type_serialize (type_serialize_params params);
err_t type_deserialize (type_deserialize_params params);
