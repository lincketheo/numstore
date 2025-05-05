#include "paging/types/inner_node.h"
#include "dev/assert.h"
#include "dev/errors.h"
#include "paging/page.h"

DEFINE_DBG_ASSERT_I (inner_node, inner_node, i)
{
  ASSERT (i);
  ASSERT (i->nkeys);
  ASSERT (i->leafs);
  ASSERT (i->keys);
  ASSERT (*i->nkeys > 0);
  // TODO - upper bounds on nkeys
}

static inline p_size
in_avail (inner_node *i)
{
  /**
   * TODO - I don't like this pointer logic
   */

  // yes, you are exceeding bounds
  u8 *start_tail = (u8 *)&i->leafs[*i->nkeys + 1];
  u8 *end_tail = (u8 *)i->keys;

  // Because it's valid
  ASSERT (start_tail <= end_tail);

  return end_tail - start_tail;
}

p_size
in_choose_lidx (const inner_node *node, b_size loc)
{
  inner_node_assert (node);

  u32 n = *node->nkeys;
  const b_size *keys = node->keys;

  for (p_size i = 0; i < n; ++i)
    {
      /**
       * keys[0]     = Last key
       * keys[n - 1] = First key
       * key[i]      = keys[n - 1 - i]
       */
      b_size key = keys[n - 1 - i];

      /**
       *                 [5]
       * [0, 1, 2, 3, 4]    [5, 6, 7, 8, 9]
       *
       * For loc == key == 5, choose right
       * So use < not <=
       */
      if (loc < key)
        {
          return i;
        }
    }

  /**
   *                 [5]
   * [0, 1, 2, 3, 4]    [5, 6, 7, 8, 9]
   *
   * For loc == 2, left = 0 (initial prev_key)
   * For loc == 8, left = 5 (last prev_key)
   */
  return n;
}

void
in_add_right (inner_node *node, p_size from, b_size add)
{
  inner_node_assert (node);
  ASSERT (from <= *node->nkeys);

  for (u32 i = from; i < *node->nkeys; ++i)
    {
      node->keys[i] += add;
    }
}

inner_node
in_set_initial_ptrs (u8 *raw, p_size len)
{
  ASSERT (raw);
  ASSERT (len > 10);

  inner_node ret;
  p_size head = 0;

  p_size header = head;
  head += sizeof (*ret.header);

  p_size nkeys = head;
  head += sizeof (*ret.nkeys);

  p_size leafs = head;
  head += sizeof (*ret.leafs);

  p_size keys = len;

  ASSERT (len > leafs);

  ret = (inner_node){
    .raw = raw,
    .rlen = len,
    .header = (pgh *)&raw[header],
    .nkeys = (p_size *)&raw[nkeys],
    .leafs = (pgno *)&raw[leafs],
    .keys = (b_size *)&raw[keys],
  };

  // ret is in an INVALID STATE!
  return ret;
}

void
in_init_empty (inner_node *in)
{
  ASSERT (in);
  *in->header = PG_INNER_NODE;
  *in->nkeys = 0;
}

err_t
in_read_and_set_ptrs (inner_node *dest, u8 *raw, p_size len)
{
  ASSERT (dest);
  ASSERT (raw);
  ASSERT (len);
  *dest = in_set_initial_ptrs (raw, len);

  p_size avail = in_avail (dest);
  p_size nkeys = *dest->nkeys;

  if (nkeys * sizeof (*dest->keys) + (nkeys + 1) * sizeof (*dest->leafs) > avail)
    {
      return ERR_INVALID_STATE;
    }

  dest->keys -= nkeys;

  inner_node_assert (dest);

  return SUCCESS;
}

void
in_init (inner_node *dest, b_size key, pgno left, pgno right)
{
  ASSERT (dest);
  ASSERT (((u8 *)dest->keys - dest->raw) == dest->rlen);
  ASSERT (*dest->nkeys == 0);
  ASSERT (in_avail (dest) >= 2 * sizeof (*dest->leafs) + sizeof (*dest->keys));

  dest->leafs[0] = left;
  dest->leafs[1] = right;

  dest->keys -= 1;
  *dest->keys = key;
  *dest->nkeys = 1;
}

bool
in_add_kv (inner_node *dest, b_size key, pgno right)
{
  inner_node_assert (dest);

  p_size avail = in_avail (dest);

  if (avail < sizeof (key) + sizeof (right))
    {
      return false;
    }

  // Grow leafs to the right
  dest->leafs[*dest->nkeys + 1] = right;

  // Grow keys to the left
  dest->keys -= 1;
  *dest->keys = key;

  // Increment nkeys
  (*dest->nkeys)++;

  return true;
}
