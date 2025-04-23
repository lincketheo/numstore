#pragma once

#include "dev/assert.h"
#include "dev/errors.h"
#include "sds.h"

/**
 * TYPE = PRIM | STRUCT | VARRAY | SARRAY | UNION | ENUM
 * STRUCT = KEY TYPE (, KEY TYPE)*
 * VARRAY = RANK TYPE
 * SARRAY = DIM (, DIM)* TYPE
 * UNION = KEY TYPE (, KEY TYPE)*
 * ENUM = KEY (, KEY)*
 */

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

  CF64 = 12,
  CF128 = 13,
  CF256 = 14,

  CI16 = 15,
  CI32 = 16,
  CI64 = 17,
  CI128 = 18,

  CU16 = 19,
  CU32 = 20,
  CU64 = 21,
  CU128 = 22,

  BOOL = 23,
  BIT = 24,
} prim_t;

DEFINE_DBG_ASSERT_H (type_t, type_t, t);
DEFINE_DBG_ASSERT_H (prim_t, prim_t, p);
u64 prim_bits_size (prim_t t);

typedef struct type type;

//////////////////////////////// Struct
typedef struct
{
  u32 len;
  string *keys;
  type *types;
} struct_t;

DEFINE_DBG_ASSERT_H (struct_t, struct_t, s);
err_t struct_t_bits_size (u64 *dest, struct_t *t);

//////////////////////////////// Union
typedef struct
{
  u32 len;
  string *keys;
  type *types;
} union_t;

DEFINE_DBG_ASSERT_H (union_t, union_t, s);
err_t union_t_bits_size (u64 *dest, union_t *t);

//////////////////////////////// Enum
typedef struct
{
  u32 len;
  string *keys;
} enum_t;

DEFINE_DBG_ASSERT_H (enum_t, enum_t, s);
err_t enum_t_bits_size (u64 *dest, enum_t *t);

//////////////////////////////// Variable Length Array
typedef struct
{
  u32 rank;
  type *t; // Not an array
} varray_t;

DEFINE_DBG_ASSERT_H (varray_t, varray_t, s);
err_t varray_t_bits_size (u64 *dest, varray_t *sa);

//////////////////////////////// Strict Array
typedef struct
{
  u32 rank;
  u32 *dims;
  type *t; // Not an array
} sarray_t;

DEFINE_DBG_ASSERT_H (sarray_t, sarray_t, s);
err_t sarray_t_bits_size (u64 *dest, sarray_t *sa);

//////////////////////////////// Generic
struct type
{
  union
  {
    prim_t p;
    struct_t *st;
    union_t *un;
    enum_t *en;
    varray_t *va;
    sarray_t *sa;
  };

  type_t type;
};

DEFINE_DBG_ASSERT_H (type, type, t);
err_t type_bits_size (u64 *dest, type *t);
