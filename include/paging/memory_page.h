#pragma once

#include "common/types.h"
#include "config.h"
#include "dev/assert.h"

#define K 10

typedef struct
{
  u8 page[PAGE_SIZE];
  u64 ptr; // whoami
  u64 lruk[K];
  int idx;
} memory_page;

DEFINE_DBG_ASSERT (memory_page, memory_page, p);

void mp_access (memory_page *m, u64 now);

u64 mp_check (const memory_page *m, u64 now);
