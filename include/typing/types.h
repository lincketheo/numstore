#pragma once

typedef enum
{
  I8,
  I16,
  I32,
  I64,
  I128,

  U8,
  U16,
  U32,
  U64,
  U128,

  F16,
  F32,
  F64,
  F128,

  CF32,
  CF64,
  CF128,
  CF256,

  CI16,
  CI32,
  CI64,
  CI128,

  CU16,
  CU32,
  CU64,
  CU128,

  CHAR,
  BOOL,
  BIT,
} prim_type;

typedef enum
{
  STRUCT,
  VARARRAY,
  STRICTARRAY,
  ENUM,
  UNION,
} adv_type;
