#pragma once

#include "core/ds/strings.h" // TODO
#include "core/intf/types.h" // TODO

typedef struct value_s value;

typedef struct
{
  u16 len;
  string *keys;
  value *values;
} struct_v;
