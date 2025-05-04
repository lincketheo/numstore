#pragma once

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

const char *prim_to_str (prim_t p);
