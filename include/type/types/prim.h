#pragma once

#include "dev/errors.h"
#include "intf/mm.h"
#include "utils/deserializer.h"
#include "utils/serializer.h"

typedef struct type_s type;

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
} prim_t;

typedef enum
{
  LUB_U64,
  LUB_I64,
  LUB_F128,
  LUB_CF256,
  LUB_CI128,
  LUB_CU128,
} lub_prim_t;

const char *prim_to_str (prim_t p);

bool prim_t_is_valid (const prim_t *t);

int prim_t_snprintf (char *str, u32 size, const prim_t *p);

u32 prim_t_byte_size (const prim_t *t);

void prim_t_serialize (serializer *dest, const prim_t *src);

err_t prim_t_deserialize (prim_t *dest, deserializer *src);
