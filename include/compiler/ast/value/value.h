#pragma once

#include "compiler/ast/type/types.h"
#include "compiler/ast/value/values/enum.h"
#include "compiler/ast/value/values/prim.h"
#include "compiler/ast/value/values/sarray.h"
#include "compiler/ast/value/values/struct.h"
#include "compiler/ast/value/values/union.h"
#include "core/intf/types.h"

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
