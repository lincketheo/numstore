#pragma once

#include "ds/strings.h"
#include "intf/types.h"

typedef struct value_s value;

typedef struct
{
  u16 len;
  string *keys;
  value *values;
} struct_v;
