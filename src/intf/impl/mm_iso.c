#include "dev/testing.h"
#include "errors/error.h"
#include "intf/logging.h"
#include "intf/mm.h"
#include "intf/stdlib.h"
#include "utils/bounds.h"

#include <errno.h>
#include <stddef.h>
#include <stdlib.h>

/////////////////////// Limited Allocator

DEFINE_DBG_ASSERT_I (lalloc, lalloc, l)
{
  ASSERT (l);
  ASSERT (l->used <= l->limit);
}

#define MIN(a, b) ((a) < (b) ? (a) : (b))

typedef struct
{
  u32 total;
  u32 elem_size;
  max_align_t _align;
  unsigned char data[];
} lalloc_block;

static inline lalloc_block *
blk_from_data (void *data)
{
  return (lalloc_block *)((char *)data - offsetof (lalloc_block, data));
}

lalloc
lalloc_create (u32 limit)
{
  ASSERT (limit > 0);
  lalloc ret = {
    .used = 0,
    .limit = limit,
  };
  lalloc_assert (&ret);
  return ret;
}

lalloc_r
lmalloc (lalloc *a, u32 req, u32 min, u32 size)
{
  lalloc_assert (a);
  ASSERT (req > 0 && min > 0 && min <= req && size > 0);

  lalloc_r ret = { .ret = NULL, .rlen = 0 };
  u32 overhead = offsetof (lalloc_block, data);
  u32 want_bytes = req * size;
  u32 min_bytes = min * size;

  if (overhead + min_bytes > a->limit)
    {
      ret.stat = AR_NOMEM;
      return ret;
    }

  u32 avail = a->limit - a->used;
  if (avail <= overhead)
    {
      ret.stat = AR_AVAIL_BUT_USED;
      return ret;
    }

  u32 payload_avail = avail - overhead;
  u32 payload = MIN (want_bytes, payload_avail);
  payload = (payload / size) * size;
  if (payload < min_bytes)
    {
      ret.stat = AR_AVAIL_BUT_USED;
      return ret;
    }

  u32 total = overhead + payload;
  lalloc_block *blk = malloc (total);
  if (!blk)
    {
      /**
       * This would be bad -
       * user exceeded system memory usage,
       * so they need to decrease memory usage fully
       */
      ret.stat = AR_NOMEM;
      return ret;
    }

  blk->total = total;
  blk->elem_size = size;
  a->used += total;

  ret.stat = AR_SUCCESS;
  ret.ret = blk->data;
  ret.rlen = payload / size;
  return ret;
}

lalloc_r
lcalloc (lalloc *a, u32 req, u32 min, u32 size)
{
  lalloc_assert (a);
  ASSERT (req > 0 && min > 0 && min <= req && size > 0);

  lalloc_r ret = { .ret = NULL, .rlen = 0 };
  u32 overhead = offsetof (lalloc_block, data);
  u32 want_bytes = req * size;
  u32 min_bytes = min * size;

  if (overhead + min_bytes > a->limit)
    {
      ret.stat = AR_NOMEM;
      return ret;
    }
  u32 avail = a->limit - a->used;
  if (avail <= overhead)
    {
      ret.stat = AR_AVAIL_BUT_USED;
      return ret;
    }
  u32 payload_avail = avail - overhead;
  u32 payload = MIN (want_bytes, payload_avail);
  payload = (payload / size) * size;
  if (payload < min_bytes)
    {
      ret.stat = AR_AVAIL_BUT_USED;
      return ret;
    }

  u32 total = overhead + payload;
  lalloc_block *blk = calloc (1, total);
  if (!blk)
    {
      // Exceeded GLOBAL memory usage,
      // not local
      ret.stat = AR_NOMEM;
      return ret;
    }

  blk->total = total;
  blk->elem_size = size;
  a->used += total;

  ret.stat = AR_SUCCESS;
  ret.ret = blk->data;
  ret.rlen = payload / size;
  return ret;
}

lalloc_r
lrealloc (lalloc *a, void *data, u32 req, u32 min, u32 size)
{
  lalloc_assert (a);
  ASSERT (data && req > 0 && min > 0 && min <= req && size > 0);

  lalloc_block *old_blk = blk_from_data (data);
  ASSERT (old_blk->elem_size == size);

  lalloc_r ret = { .ret = NULL, .rlen = 0 };
  u32 old_total = old_blk->total;
  u32 overhead = offsetof (lalloc_block, data);
  u32 want_bytes = req * size;
  u32 min_bytes = min * size;

  if (overhead + min_bytes > a->limit)
    {
      ret.stat = AR_NOMEM;
      return ret;
    }

  u32 avail = a->limit - (a->used - old_total);
  if (avail <= overhead)
    {
      ret.stat = AR_AVAIL_BUT_USED;
      return ret;
    }

  u32 payload_avail = avail - overhead;
  u32 payload = MIN (want_bytes, payload_avail);
  payload = (payload / size) * size;
  if (payload < min_bytes)
    {
      ret.stat = AR_AVAIL_BUT_USED;
      return ret;
    }

  u32 total = overhead + payload;
  lalloc_block *blk = realloc (old_blk, total);
  if (!blk)
    {
      // Exceeded GLOBAL memory usage,
      // not local
      ret.stat = AR_NOMEM;
      return ret;
    }

  blk->total = total;
  a->used = a->used - old_total + total;

  ret.stat = AR_SUCCESS;
  ret.ret = blk->data;
  ret.rlen = payload / size;
  return ret;
}

void
lfree (lalloc *a, void *data)
{
  ASSERT (data);
  lalloc_block *blk = blk_from_data (data);
  ASSERT (a->used > blk->total);
  a->used -= blk->total;
  free (blk);
}

#ifndef NTEST
void *
lmalloc_test (lalloc *a, u32 n, u32 size)
{
  lalloc_r ret = lmalloc (a, n, n, size);
  if (ret.stat == AR_SUCCESS)
    {
      ASSERT (ret.ret);
      return ret.ret;
    }

  ASSERT (!ret.ret);
  return NULL;
}

void *
lcalloc_test (lalloc *a, u32 n, u32 size)
{
  lalloc_r ret = lcalloc (a, n, n, size);
  if (ret.stat == AR_SUCCESS)
    {
      ASSERT (ret.ret);
      return ret.ret;
    }

  ASSERT (!ret.ret);
  return NULL;
}
#endif
