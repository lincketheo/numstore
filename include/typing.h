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
typedef struct type_subset type_subset;

//////////////////////////////// Struct
typedef struct
{
  u32 len;
  string *keys;
  type *types;
} struct_t;

typedef struct
{
  u32 len;
  string *keys;
  type_subset *types;

  // Selected keys
  u32 *sel;
  u32 lsel;
} struct_subset_t;

DEFINE_DBG_ASSERT_H (struct_t, struct_t, s);
DEFINE_DBG_ASSERT_H (struct_subset_t, struct_subset_t, s);
err_t struct_t_bits_size (u64 *dest, struct_t *t);

//////////////////////////////// Strict Array
typedef struct
{
  u32 rank;
  u32 *dims;
  type *t; // Not an array
} sarray_t;

// Indexed Array
typedef struct
{
  u32 rank;
  u32 *dims;
  type_subset *t; // Not an array

  array_range *ranges;
} sarray_subset_t;

DEFINE_DBG_ASSERT_H (sarray_t, sarray_t, s);
DEFINE_DBG_ASSERT_H (sarray_subset_t, sarray_subset_t, s);
err_t sarray_t_bits_size (u64 *dest, sarray_t *sa);

//////////////////////////////// Generic
struct type
{
  union
  {
    prim_t p;
    struct_t *st;
    sarray_t *sa;
  };

  type_t type;
};

struct type_subset
{
  union
  {
    prim_t p;
    struct_subset_t *st;
    sarray_subset_t *sa;
  };

  type_t type;
};

DEFINE_DBG_ASSERT_H (type, type, t);
DEFINE_DBG_ASSERT_H (type_subset, type_subset, t);
err_t type_bits_size (u64 *dest, type *t);
