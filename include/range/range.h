#pragma once

#include "intf/types.h"

typedef struct
{
  b_size start;
  b_size stop;
  b_size step;
} range;

/**
 * Signed range ([-1:10:2])
 */
typedef struct
{
  sb_size start;
  sb_size stop;
  sb_size step;
} srange;
