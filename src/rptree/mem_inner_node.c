#include "rptree/mem_inner_node.h"
#include "dev/errors.h"
#include "intf/mm.h"
#include "intf/stdlib.h"
#include "paging/types/inner_node.h"

DEFINE_DBG_ASSERT_I (mem_inner_node, mem_inner_node, o)
{
  ASSERT (o);
  //  ASSERT (o->first_pg != 0); // TODO - enable
  ASSERT (o->kvlen <= o->kvcap);
  if (o->kvcap == 0)
    {
      ASSERT (o->kvs == NULL);
    }
  else
    {
      ASSERT (o->kvs);
    }
}

mem_inner_node
mintn_create (mintn_params params)
{
  mem_inner_node ret = {
    .first_pg = params.pg,
    .kvcap = 0,
    .kvlen = 0,
    .alloc = params.alloc,
    .kvs = NULL,
  };
  mem_inner_node_assert (&ret);
  return ret;
}

err_t
mintn_clip (mem_inner_node *r)
{
  mem_inner_node_assert (r);

  if (r->kvs == NULL)
    {
      return SUCCESS;
    }

  pg_rk *kvs = lrealloc (r->alloc, r->kvs, r->kvlen * sizeof *kvs);
  if (!kvs)
    {
      // This is probably an IO_ERR
      return ERR_NOMEM;
    }
  r->kvs = kvs;
  r->kvcap = r->kvlen;
  return SUCCESS;
}

void
mintn_free (mem_inner_node *r)
{
  mem_inner_node_assert (r);

  if (r->kvs == NULL)
    {
      return;
    }

  lfree (r->alloc, r->kvs);

  r->kvlen = 0;
  r->kvcap = 0;
  r->kvs = NULL;
}

static inline err_t
mintn_make_room (mem_inner_node *r, u32 cap)
{
  mem_inner_node_assert (r);
  ASSERT (cap > 0);

  pg_rk *kvs = NULL;

  if (r->kvs == NULL)
    {
      kvs = lmalloc (r->alloc, cap * sizeof *kvs);
    }
  else
    {
      kvs = lrealloc (r->alloc, r->kvs, cap * sizeof *kvs);
    }

  if (!kvs)
    {
      return ERR_NOMEM;
    }

  r->kvcap = cap;
  r->kvs = kvs;

  return SUCCESS;
}

err_t
mintn_add_right (mem_inner_node *r, b_size key, pgno pg)
{
  mem_inner_node_assert (r);

  if (r->kvlen == r->kvcap)
    {
      err_t ret = mintn_make_room (r, r->kvcap == 0 ? 10 : r->kvcap * 2);
      if (ret)
        {
          return ret;
        }
    }

  /*
   * compute cumulative key
   * add to the previous far-right key if any
   */
  b_size adj_key = key;
  if (r->kvlen > 0)
    {
      adj_key += r->kvs[r->kvlen - 1].key;
    }

  r->kvs[r->kvlen++] = (pg_rk){
    .pg = pg,
    .key = adj_key,
  };

  return SUCCESS;
}

err_t
mintn_add_right_no_add (mem_inner_node *r, b_size key, pgno pg)
{
  mem_inner_node_assert (r);

  if (r->kvlen == r->kvcap)
    {
      err_t ret = mintn_make_room (r, r->kvcap == 0 ? 10 : r->kvcap * 2);
      if (ret)
        {
          return ret;
        }
    }

  r->kvs[r->kvlen++] = (pg_rk){
    .pg = pg,
    .key = key,
  };

  return SUCCESS;
}

err_t
mintn_add_left (mem_inner_node *r, pgno pg, b_size key)
{
  mem_inner_node_assert (r);

  if (r->kvlen == r->kvcap)
    {
      err_t ret = mintn_make_room (r, r->kvcap == 0 ? 10 : r->kvcap * 2);
      if (ret)
        {
          return ret;
        }
    }

  /**
   * shift existing kvs right, and add to each shifted key
   */
  p_size old_len = r->kvlen;
  if (old_len > 0)
    {
      memmove (
          &r->kvs[1],
          &r->kvs[0],
          old_len * sizeof *r->kvs);
      for (p_size i = 0; i < old_len; ++i)
        {
          r->kvs[i + 1].key += key;
        }
    }

  /*
   * insert new separator at 0, using old first_pg as its right‐child
   */
  pgno old_first = r->first_pg;
  r->kvs[0] = (pg_rk){
    .pg = old_first,
    .key = key,
  };
  r->first_pg = pg;

  r->kvlen++;
  return SUCCESS;
}

b_size
mintn_get_left (mem_inner_node *r, pgno exp)
{
  mem_inner_node_assert (r);
  ASSERT (exp == r->first_pg);
  ASSERT (r->kvlen > 0);

  /*
   * size of the removed leftmost subtree
   */
  b_size removed = r->kvs[0].key;
  pgno new_first = r->kvs[0].pg;

  /*
   * shift them down and adjust keys
   */
  if (r->kvlen > 1)
    {
      i_memmove (
          &r->kvs[0],
          &r->kvs[1],
          (r->kvlen - 1) * sizeof *r->kvs);
      for (p_size i = 0; i < r->kvlen - 1; ++i)
        {
          r->kvs[i].key -= removed;
        }
    }

  r->kvlen -= 1;
  r->first_pg = new_first;
  return removed;
}

void
mintn_write_max_into_in (inner_node *dest, mem_inner_node *m)
{
  mem_inner_node_assert (m);

  if (in_get_nkeys (dest) == 0)
    {
      pgno left = m->first_pg;
      b_size key = mintn_get_left (m, m->first_pg);
      in_init (dest, key, left, m->first_pg);
    }

  // TODO - can be heavily optimized (see memcpies)
  while (m->kvlen > 0 && in_keys_avail (dest) > 0)
    {
      b_size key = mintn_get_left (m, in_get_right_most_leaf (dest));
      key += in_get_right_most_key (dest);
      bool added = in_add_kv (dest, key, m->first_pg);
      ASSERT (added);
    }
}
