#pragma once

#include "core/intf/types.h" // TODO

#include "compiler/ast/type/types.h"          // TODO
#include "compiler/ast/value/values/enum.h"   // TODO
#include "compiler/ast/value/values/prim.h"   // TODO
#include "compiler/ast/value/values/sarray.h" // TODO
#include "compiler/ast/value/values/struct.h" // TODO
#include "compiler/ast/value/values/union.h"  // TODO

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
