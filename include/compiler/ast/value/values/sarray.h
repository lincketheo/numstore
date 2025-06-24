#pragma once

#include "core/ds/strings.h" // TODO

typedef struct value_s value;

typedef struct
{
  u32 len;
  value *values;
} sarray_v;
