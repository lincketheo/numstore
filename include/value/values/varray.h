#pragma once

#include "ds/strings.h"

typedef struct value_s value;

typedef struct
{
  value *values;
  u32 *dims;
  u32 rank;
} varray_v;
