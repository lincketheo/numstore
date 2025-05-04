#pragma once

#include "ds/strings.h"

typedef struct value_s value;

typedef struct
{
  string key;
  value *value;
} union_v;
