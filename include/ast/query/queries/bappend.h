#pragma once

#include "ds/cbuffer.h"
#include "ds/strings.h"
#include "mm/lalloc.h"

/**
 * append 5 [a, (b, c)] DATA
 */
typedef struct
{
  string vname;
  b_size nelem;

  u8 _input[10];
  cbuffer input;

  lalloc query_allocator;
  u8 _query_allocator[2048];
} bappend_query;
