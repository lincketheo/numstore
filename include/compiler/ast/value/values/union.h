#pragma once

#include "core/ds/strings.h" // TODO

typedef struct value_s value;

typedef struct
{
  string key;
  value *value; // A single value
} union_v;
