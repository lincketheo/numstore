#pragma once

#include "core/ds/strings.h"
#include "core/errors/error.h"
#include "core/mm/lalloc.h"

typedef struct value_s value;

typedef struct
{
  value *values;
  u32 len;
} array;

bool array_equal (const array *left, const array *right);
err_t array_plus (array *dest, const array *right, lalloc *alloc, error *e);
