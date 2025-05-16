#pragma once

#include "ast/type/types.h"
#include "ast/value/values/enum.h"
#include "ast/value/values/prim.h"
#include "ast/value/values/sarray.h"
#include "ast/value/values/struct.h"
#include "ast/value/values/union.h"
#include "intf/types.h"

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
