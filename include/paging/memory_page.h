#pragma once

#include "config.h"
#include "dev/assert.h"
#include "os/types.h"

typedef struct
{
  u8 page[PAGE_SIZE];
  u64 ptr; // whoami
  u64 laccess;
} memory_page;

DEFINE_DBG_ASSERT_H (memory_page, memory_page, p);

void mp_create (memory_page *dest, u64 ptr, u64 clock);

void mp_access (memory_page *m, u64 now);

u64 mp_check (const memory_page *m, u64 now);
