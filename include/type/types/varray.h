#pragma once

#include "ds/strings.h"
#include "intf/types.h"

typedef struct type_s type;

typedef struct
{
  u16 rank;
  type *t; // Not an array
} varray_t;
