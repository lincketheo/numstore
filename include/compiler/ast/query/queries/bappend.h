#pragma once

#include "core/ds/cbuffer.h"
#include "core/ds/strings.h"
#include "core/mm/lalloc.h"

/**
 * bappend 5 [a, (b, c)] DATA
 */

typedef struct
{
  struct
  {
    string *vnames;
    u32 len;
  } neighbors;
  u32 nlen;
} bappend_vars;

typedef struct
{
  bappend_vars vars;
  b_size nelem;

  u8 _input[10];
  cbuffer input;

  lalloc query_allocator;
  u8 _query_allocator[2048];
} bappend_query;
