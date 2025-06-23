#pragma once

#include "compiler/ast/type/types/prim.h" // prim_t

typedef struct
{
  prim_t label;

  union
  {
    u8 _u8;
    u16 _u16;
    u32 _u32;
    u64 _u64;

    i8 _i8;
    i16 _i16;
    i32 _i32;
    i64 _i64;

    f16 _f16;
    f32 _f32;
    f64 _f64;
    f128 _f128;

    cf32 _cf32;
    cf64 _cf64;
    cf128 _cf128;
    cf256 _cf256;

    ci16 _ci16;
    ci32 _ci32;
    ci64 _ci64;
    ci128 _ci128;

    cu16 _cu16;
    ci32 _cu32;
    cu64 _cu64;
    cu128 _cu128;
  };
} prim_v;

typedef struct
{
  lub_prim_t label;

  union
  {
    u64 _u64;
    i64 _i64;
    f128 _f128;
    cf256 _cf256;
    ci128 _ci128;
    cu128 _cu128;
  };
} lub_prim_v;
