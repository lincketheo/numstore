#include "numstore/rptree/mem_inner_node.h"

#include "core/dev/assert.h"   // DEFINE_DBG_ASSERT_I
#include "core/errors/error.h" // error
#include "core/intf/stdlib.h"  // i_memcpy
#include "core/mm/lalloc.h"    // malloc

#include "numstore/paging/types/inner_node.h" // inner_node

DEFINE_DBG_ASSERT_I (mem_inner_node, mem_inner_node, o)
{
  ASSERT (o);
  ASSERT (o->klen <= 50);
}

void
meminode_create (mem_inner_node *dest, pgno pg)
{
  dest->klen = 0;
  dest->values[0] = pg;
  mem_inner_node_assert (dest);
}

u32
meminode_avail (mem_inner_node *r)
{
  mem_inner_node_assert (r);
  return 50 - r->klen;
}

bool
meminode_full (mem_inner_node *r)
{
  mem_inner_node_assert (r);
  return r->klen == 50;
}

void
meminode_push_right (mem_inner_node *r, b_size key, pgno pg)
{
  mem_inner_node_assert (r);

  ASSERT (r->klen < 50);

  /*
   * compute cumulative key
   * push to the previous far-right key if any
   */
  if (r->klen > 0)
    {
      key += r->keys[r->klen - 1];
    }

  meminode_push_right_no_add (r, key, pg);
}

void
meminode_push_right_no_add (mem_inner_node *r, b_size key, pgno pg)
{
  mem_inner_node_assert (r);

  ASSERT (r->klen < 50);

  r->keys[r->klen] = key;
  r->values[r->klen + 1] = pg;

  r->klen++;
}

void
meminode_push_left (mem_inner_node *r, pgno pg, b_size key)
{
  mem_inner_node_assert (r);
  ASSERT (r->klen < 50);

  // Add [key] to all right most nodes
  for (p_size i = 0; i < r->klen; ++i)
    {
      r->keys[i + 1] += key;
    }

  meminode_push_left_no_add (r, pg, key);
}

void
meminode_push_left_no_add (mem_inner_node *r, pgno pg, b_size key)
{
  mem_inner_node_assert (r);
  ASSERT (r->klen < 50);

  /**
   * shift existing keys right, and push to each shifted key
   */
  if (r->klen)
    {
      // Shift arrays right
      i_memmove (
          &r->keys[1],
          &r->keys[0],
          r->klen * sizeof *r->keys);

      i_memmove (
          &r->values[1],
          &r->values[0],
          (r->klen + 1) * sizeof *r->values);
    }

  r->keys[0] = key;
  r->values[0] = pg;

  r->klen += 1;
}

meminode_kv
meminode_pop_left (mem_inner_node *r, pgno exp)
{
  mem_inner_node_assert (r);

  ASSERT (exp == r->values[0]);
  ASSERT (r->klen > 0);

  /*
   * size of the removed leftmost subtree
   */
  b_size left = r->keys[0];
  b_size pg = r->values[0];

  /*
   * shift them down and adjust keys
   */
  if (r->klen > 1)
    {
      // Shift left
      i_memmove (
          &r->keys[0],
          &r->keys[1],
          (r->klen - 1) * sizeof *r->keys);

      i_memmove (
          &r->values[0],
          &r->values[1],
          (r->klen) * sizeof *r->values);

      // Subtract [left]
      for (p_size i = 0; i < r->klen - 1; ++i)
        {
          ASSERT (r->keys[i] > left);
          r->keys[i] -= left;
        }
    }

  r->klen -= 1;

  return (meminode_kv){
    .key = left,
    .value = pg,
  };
}

void
meminode_write_max (inner_node *dest, mem_inner_node *m)
{
  mem_inner_node_assert (m);
  ASSERT (m->klen > 0);

  /**
   * On empty inner node, write out
   * the first 2 values (1 key)
   */
  if (in_get_nkeys (dest) == 0)
    {
      meminode_kv left = meminode_pop_left (m, m->values[0]);
      in_init (dest, left.key, left.value, m->values[0]);
    }

  // TODO - can be heavily optimized (see memcpies)
  while (m->klen > 0 && in_keys_avail (dest) > 0)
    {
      b_size right = in_get_right_most_key (dest);

      meminode_kv left = meminode_pop_left (m, right);
      left.key += right;

      bool pushed = in_add_kv (dest, left.key, m->values[0]);
      ASSERT (pushed);
    }
}
