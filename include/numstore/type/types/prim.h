#pragma once

#include "core/errors/error.h"       // error
#include "core/intf/types.h"         // u32
#include "core/utils/deserializer.h" // deserializer
#include "core/utils/serializer.h"   // serializer

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

#define PRIM_FOR_EACH(func, ...) \
  do                             \
    {                            \
      func (U8, __VA_ARGS__);    \
      func (U8, __VA_ARGS__);    \
      func (U16, __VA_ARGS__);   \
      func (U32, __VA_ARGS__);   \
      func (U64, __VA_ARGS__);   \
      func (I8, __VA_ARGS__);    \
      func (I16, __VA_ARGS__);   \
      func (I32, __VA_ARGS__);   \
      func (I64, __VA_ARGS__);   \
      func (F16, __VA_ARGS__);   \
      func (F32, __VA_ARGS__);   \
      func (F64, __VA_ARGS__);   \
      func (F128, __VA_ARGS__);  \
      func (CF32, __VA_ARGS__);  \
      func (CF64, __VA_ARGS__);  \
      func (CF128, __VA_ARGS__); \
      func (CF256, __VA_ARGS__); \
      func (CI16, __VA_ARGS__);  \
      func (CI32, __VA_ARGS__);  \
      func (CI64, __VA_ARGS__);  \
      func (CI128, __VA_ARGS__); \
      func (CU16, __VA_ARGS__);  \
      func (CU32, __VA_ARGS__);  \
      func (CU64, __VA_ARGS__);  \
      func (CU128, __VA_ARGS__); \
    }                            \
  while (0)

#define PRIM_FOR_EACH_LITERAL(func, ...) \
  do                                     \
    {                                    \
      func ("u8", __VA_ARGS__);          \
      func ("u8", __VA_ARGS__);          \
      func ("u16", __VA_ARGS__);         \
      func ("u32", __VA_ARGS__);         \
      func ("u64", __VA_ARGS__);         \
      func ("i8", __VA_ARGS__);          \
      func ("i16", __VA_ARGS__);         \
      func ("i32", __VA_ARGS__);         \
      func ("i64", __VA_ARGS__);         \
      func ("f16", __VA_ARGS__);         \
      func ("f32", __VA_ARGS__);         \
      func ("f64", __VA_ARGS__);         \
      func ("f128", __VA_ARGS__);        \
      func ("cf32", __VA_ARGS__);        \
      func ("cf64", __VA_ARGS__);        \
      func ("cf128", __VA_ARGS__);       \
      func ("cf256", __VA_ARGS__);       \
      func ("ci16", __VA_ARGS__);        \
      func ("ci32", __VA_ARGS__);        \
      func ("ci64", __VA_ARGS__);        \
      func ("ci128", __VA_ARGS__);       \
      func ("cu16", __VA_ARGS__);        \
      func ("cu32", __VA_ARGS__);        \
      func ("cu64", __VA_ARGS__);        \
      func ("cu128", __VA_ARGS__);       \
    }                                    \
  while (0)

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
err_t prim_t_validate (const prim_t *t, error *e);
i32 prim_t_snprintf (char *str, u32 size, const prim_t *p);
u32 prim_t_byte_size (const prim_t *t);
void prim_t_serialize (serializer *dest, const prim_t *src);
err_t prim_t_deserialize (prim_t *dest, deserializer *src, error *e);
prim_t prim_t_random (void);
