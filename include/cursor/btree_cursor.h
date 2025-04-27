#pragma once

#include "dev/assert.h"
#include "ds/cbuffer.h"
#include "intf/mm.h"
#include "paging/page.h"
#include "paging/pager.h"
#include "utils/numbers.h"

typedef struct
{
  page page;
  u64 bidx;
} btc_pgctx;

typedef struct
{
  page root;
  page cur_page;
  u64 idx;

  bool is_page_loaded;
  bool is_seeked;

  pager *pager;
  lalloc *stack_allocator;
} btree_cursor;

typedef struct
{
  pager *pager;
  lalloc *stack_allocator;
} btc_params;

typedef enum
{
  ST_SET,
  ST_CUR,
  ST_END,
} seek_t;

// Create
void btc_create (btree_cursor *dest, btc_params params);

// Open / close
err_t btc_open (btree_cursor *dest, u64 p0);
err_t btc_close (btree_cursor *n);

// Seeking
err_t btc_seek (btree_cursor *n, u64 size, u64 *nelem, seek_t whence);
err_t btc_rewind (btree_cursor *n);
err_t btc_eof (btree_cursor *n);
err_t btc_forward (btree_cursor *n, u64 size, u64 *nelem);
err_t btc_backward (btree_cursor *n, u64 size, u64 *nelem);

// Read / write
err_t btc_read (cbuffer *dest, btree_cursor *src, u64 size, u64 *nelem);
err_t btc_write (cbuffer *src, btree_cursor *dest, u64 size, u64 *nelem);
