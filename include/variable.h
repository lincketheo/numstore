#pragma once

#include "dev/assert.h"
#include "ds/strings.h"
#include "type/types.h"

////////////////////////////// A Single Variable
typedef struct
{
  u64 pgn0;
  type type;
  u64 vid;
} vmeta;
