#pragma once

#include "dev/assert.h"
#include "os/types.h"

typedef struct
{
  u8 *data;
  u32 cap;
  u32 head;
  u32 tail;
  int isfull;
} cbuffer;

cbuffer cbuffer_create (u8 *data, u32 cap);

u32 cbuffer_len (const cbuffer *b);

static inline DEFINE_DBG_ASSERT_I (cbuffer, cbuffer, b)
{
  ASSERT (b);
  ASSERT (b->cap > 0);
  ASSERT (b->data);
  if (b->isfull)
    {
      ASSERT (b->tail == b->head);
    }
  ASSERT (cbuffer_len (b) <= b->cap);
}

u32 cbuffer_read (void *dest, u32 size, u32 n, cbuffer *b);

u32 cbuffer_write (const void *dest, u32 size, u32 n, cbuffer *b);

typedef struct
{
  cbuffer *bufs;
  int *is_present;
  u32 len;
  u32 idx;
} cbuffer_mgr;

static inline DEFINE_DBG_ASSERT_I (cbuffer_mgr, cbuffer_mgr, b)
{
  ASSERT (b);
  ASSERT (b->bufs);
  ASSERT (b->is_present);
  ASSERT (b->len > 0);
  ASSERT (b->idx > 0);
  ASSERT (b->idx < b->len);
}

cbuffer_mgr bmgr_create (cbuffer *bufs, int *is_present, u32 len);

cbuffer *bmgr_get (cbuffer_mgr *bufs, int *idx);

void bmgr_release (cbuffer_mgr *bufs, u32 idx);
