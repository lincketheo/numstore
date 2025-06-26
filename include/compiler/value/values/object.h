#pragma once

#include "core/ds/strings.h"

typedef struct value_s value;

typedef struct
{
  string *keys;
  value *values;
  u32 len;
} object;

i32 object_t_snprintf (char *str, u32 size, const object *st);
