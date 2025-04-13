#pragma once

#include "dev/assert.h"
#include "os/types.h"

/////// RANGES
typedef struct
{
  u64 start;
  u64 end;
  u64 span;
} srange;

static inline DEFINE_DBG_ASSERT (srange, srange, s)
{
  ASSERT (s);
  ASSERT (s->start <= s->end);
}

// Copies data from [src] into [dest] using range defined by [range]
// [dnelem] is the capacity of dest - data is appended contiguously
// [snelem] is the number of elements in src
u64 srange_copy (
    u8 *dest,
    u64 dnelem,
    const u8 *src,
    u64 snelem,
    srange range,
    u64 size);

/////// STRINGS
typedef struct
{
  char *data;
  int len;
} string;

static inline DEFINE_DBG_ASSERT (string, string, s)
{
  ASSERT (s);
  ASSERT (s->data);
  ASSERT (s->len > 0);
}

static inline DEFINE_DBG_ASSERT (string, cstring, s)
{
  string_assert (s);
  ASSERT (s->data[s->len] == 0);
}
