#pragma once

#include "intf/mm.h"
#include "intf/types.h"
#include "paging/pager.h"
#include "paging/types/hash_leaf.h"

typedef struct
{
  page cur;

  pager *pager;

  struct
  {
    b_size gidx;
    b_size lidx;
    bool is_open;
    bool is_seeked;
  };

  u8 *temp_mem;
  p_size tmlen;
  p_size tmcap;
  lalloc *alloc;
} rptree;

typedef struct
{
  pager *pager;
  lalloc *alloc;
  p_size page_size;
} rpt_params;

err_t rpt_create (rptree *dest, rpt_params params);

err_t rpt_new (rptree *r, pgno *p0);
err_t rpt_open (rptree *r, pgno p0);
void rpt_close (rptree *r);

err_t rpt_seek (rptree *r, t_size size, b_size n);
err_t rpt_read (u8 *dest, t_size size, b_size *n, b_size nskip, rptree *r);
err_t rpt_insert (const u8 *src, t_size size, b_size n, rptree *r);
