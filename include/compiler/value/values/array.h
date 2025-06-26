#pragma once

#include "core/ds/strings.h"

typedef struct value_s value;

typedef struct
{
  value *values;
  u32 len;
} array;
