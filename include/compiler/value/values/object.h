#pragma once

#include "core/ds/strings.h"
#include "core/errors/error.h"
#include "core/mm/lalloc.h"

typedef struct value_s value;

typedef struct
{
  string *keys;
  value *values;
  u32 len;
} object;

i32 object_t_snprintf (char *str, u32 size, const object *st);
bool object_equal (const object *left, const object *right);
err_t object_plus (object *dest, const object *right, lalloc *alloc, error *e);
