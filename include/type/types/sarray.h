#pragma once

#include "ds/strings.h"
#include "intf/types.h"

typedef struct type_s type;

typedef struct
{
  u16 rank;
  u32 *dims;
  type *t; // Not an array
} sarray_t;
