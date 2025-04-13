#pragma once

#include "config.h"
#include "dev/assert.h"
#include "os/io.h"
#include "os/types.h"

/**
 * A pager that finds pages directly from a file
 */
typedef struct
{
  i_file *f;
} file_pager;

static inline DEFINE_DBG_ASSERT_I (file_pager, file_pager, p)
{
  ASSERT (p);
  i_file_assert (p->f);
}

int fpgr_new (file_pager *p, u64 *dest);

int fpgr_delete (file_pager *p, u64 ptr);

int fpgr_get (file_pager *p, u8 dest[PAGE_SIZE], u64 ptr);

int fpgr_commit (file_pager *p, const u8 src[PAGE_SIZE], u64 ptr);
