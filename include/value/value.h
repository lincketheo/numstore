#pragma once

#include "intf/types.h"
#include "type/types.h"
#include "value/values/enum.h"
#include "value/values/prim.h"
#include "value/values/sarray.h"
#include "value/values/struct.h"
#include "value/values/union.h"

struct value_s
{
  type_t type;
  union
  {
    prim_v p;
    struct_v st;
    union_v un;
    enum_v en;
    sarray_v sa;
  };
};
