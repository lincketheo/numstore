#pragma once

#include "ns_errors.h"
#include "ns_types.h"

///////////////////////////////
////////// srange

typedef enum
{
  U8 = 0,
  I8 = 1,

  U16 = 2,
  I16 = 3,

  U32 = 4,
  I32 = 5,

  U64 = 6,
  I64 = 7,

  F16 = 8,
  F32 = 9,
  F64 = 10,

  CF32 = 11,
  CF64 = 12,
  CF128 = 13,

  CI16 = 14,
  CI32 = 15,
  CI64 = 16,
  CI128 = 17,

  CU16 = 18,
  CU32 = 19,
  CU64 = 20,
  CU128 = 21,
} dtype;

#define dtype_max 21

ns_size dtype_sizeof (dtype d);

ns_ret_t inttodtype (dtype *dest, int val);
