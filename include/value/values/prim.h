#pragma once

#include "typing.h"

typedef struct
{
  prim_t label;

  union
  {
    u64 uinteger;
    i64 iinteger;
    f128 floating;
    f128 fcomplex[2];
    u64 ucomplex[2];
    i64 icomplex[2];
  };
} prim_v;
