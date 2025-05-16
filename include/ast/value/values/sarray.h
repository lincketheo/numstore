#pragma once

#include "ds/strings.h"

typedef struct value_s value;

typedef struct
{
  u32 len;
  value *values;
} sarray_v;
