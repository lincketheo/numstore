#pragma once

#include "dev/assert.h"
#include "os/types.h"

typedef enum {
  U8,
  U16,
  U32,
  U64,

  I8,
  I16,
  I32,
  I64,

  F32,
  F64,
  F128,

  CF64,
  CF128,
  CF256,

  CI16,
  CI32,
  CI64,
  CI128,

  CU16,
  CU32,
  CU64,
  CU128,

  CHAR,
  BOOL,
} dtype;

u64 dtype_sizeof(const dtype type);

/////// Internal Node Types
typedef u64 page_ptr;
typedef u16 offset_t;
typedef u16 keylen_t;
typedef u16 nkeys_t;

#define PAGE_SIZE 4096

/////// RANGES
typedef struct
{
  u64 start;
  u64 end;
  u64 span;
} srange;

static inline int srange_valid(const srange* s)
{
  return s->start <= s->end;
}

DEFINE_ASSERT(srange, srange)

// Copies data from [src] into [dest] using range defined by [range]
// [dnelem] is the capacity of dest - data is appended contiguously
// [snelem] is the number of elements in src
u64 srange_copy(
  u8* dest,
  u64 dnelem,
  const u8* src,
  u64 snelem,
  srange range,
  u64 size);

/////// STRINGS
typedef struct {
  char* data;
  int len;
} string;
