#pragma once

#include "ns_types.h"

///////////////////////////////
////////// srange

typedef struct
{
  ns_size start;
  ns_size end;
  ns_size stride;
  int isinf;
} ssrange;

#define ssrange_ASSERT(s)                                                     \
  ASSERT (s.start <= s.end);                                                  \
  ASSERT (s.stride > 0)

typedef struct
{
  ns_size start;
  ns_size end;
  int isinf;
} srange;

#define srange_ASSERT(s) ASSERT (s.start <= s.end);

// Constructors
#define ssrange_from_num(n)                                                   \
  (ssrange) { .start = 0, .end = n, .stride = 1, .isinf = 0 }
