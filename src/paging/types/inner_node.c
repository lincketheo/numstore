#include "paging/types/inner_node.h"
#include "dev/assert.h"
#include "dev/errors.h"
#include "paging/page.h"
#include "paging/types/data_list.h"

#define IN_HEDR_OFFSET (0)
#define IN_NKEY_OFFSET (IN_HEDR_OFFSET + sizeof (*((inner_node *)0)->header))
#define IN_LEAF_OFFSET (IN_NKEY_OFFSET + sizeof (*((inner_node *)0)->nkeys))

DEFINE_DBG_ASSERT_I (inner_node, unchecked_inner_node, i)
{
  ASSERT (i);
  ASSERT_PTR_IS_IDX (i->raw, i->header, IN_HEDR_OFFSET);
  ASSERT_PTR_IS_IDX (i->raw, i->nkeys, IN_NKEY_OFFSET);
  ASSERT_PTR_IS_IDX (i->raw, i->leafs, IN_LEAF_OFFSET);

  // Check that keys is pointing at the right spot
  if (i->keys)
    {
      ASSERT ((u8 *)i->keys > (u8 *)i->raw);
      p_size keys_start = (u8 *)i->keys - (u8 *)i->raw;
      ASSERT (keys_start < i->rlen);
    }
}

DEFINE_DBG_ASSERT_I (inner_node, valid_inner_node, i)
{
  ASSERT (in_is_valid (i));
}

bool
in_is_valid (const inner_node *in)
{
  unchecked_inner_node_assert (in);

  /**
   * Check that nkeys is less than max keys
   */
  if (*in->nkeys > dl_data_size (in->rlen))
    {
      return false;
    }

  /**
   * Inner nodes must contain at least
   * 1 key
   */
  if (*in->nkeys == 0)
    {
      return false;
    }

  if (in->keys == NULL)
    {
      return false;
    }

  /**
   * The distance between keys and the
   * end should be exactly nkeys
   */
  p_size keys_start = (u8 *)in->keys - (u8 *)in->raw;
  ASSERT (keys_start < in->rlen); // From previous assert
  if ((in->rlen - keys_start) != *in->nkeys / sizeof (b_size))
    {
      return false;
    }

  return true;
}

p_size
in_choose_lidx (const inner_node *node, b_size loc)
{
  valid_inner_node_assert (node);

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
  valid_inner_node_assert (node);
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

  ASSERT (raw);
  ASSERT (len > 0);

  inner_node ret;

  // Set pointers
  ret = (inner_node){
    .raw = raw,
    .rlen = len,
    .header = (pgh *)&raw[IN_HEDR_OFFSET],
    .nkeys = (p_size *)&raw[IN_NKEY_OFFSET],
    .leafs = (pgno *)&raw[IN_LEAF_OFFSET],
    .keys = NULL,
  };

  unchecked_inner_node_assert (&ret);

  // We are in an INVALID state because we haven't read anything yet

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

  // First set pointers to initial positions
  *dest = in_set_initial_ptrs (raw, len);

  unchecked_inner_node_assert (dest);

  // Then adjust keys
  p_size nkeys = *dest->nkeys;
  if (nkeys > in_max_nkeys (len))
    {
      return ERR_INVALID_STATE;
    }
  if (nkeys == 0)
    {
      return ERR_INVALID_STATE;
    }
  dest->keys = (b_size *)(&raw[len - nkeys]);

  // One last validitiy check (in other things)
  if (!in_is_valid (dest))
    {
      return ERR_INVALID_STATE;
    }

  valid_inner_node_assert (dest);

  return SUCCESS;
}

void
in_init (inner_node *dest, b_size key, pgno left, pgno right)
{
  unchecked_inner_node_assert (dest);
  dest->leafs[0] = left;
  dest->leafs[1] = right;

  dest->keys = (b_size *)(&dest->raw[dest->rlen - 1]);
  dest->keys[0] = key;
  *dest->nkeys = 1;
  valid_inner_node_assert (dest);
}

bool
in_add_kv (inner_node *dest, b_size key, pgno right)
{
  valid_inner_node_assert (dest);

  if (*dest->nkeys == in_max_nkeys (dest->rlen))
    {
      return false;
    }

  // Grow leafs to the right
  dest->leafs[(*dest->nkeys)++] = right;

  // Grow keys to the left
  dest->keys -= 1;
  dest->keys[0] = key;

  return true;
}

p_size
in_max_nkeys (p_size page_size)
{
  /**
   * nbytes = sizeof(
   * -------------
   * LEAF0
   * LEAF1
   * ....
   * KEY1
   * KEY0
   * -------------
   * )
   */

  // header + nkeys
  ASSERT (page_size > IN_LEAF_OFFSET);
  p_size nbytes = page_size - IN_LEAF_OFFSET;

  /**
   * N * sizeof(key) + (N + 1) * sizeof(value) <= nbytes
   * N * (sizeof(key) + sizeof(value)) + sizeof(value) <= nbytes
   * N <= (nbytes - sizeof(value))/(sizeof(key) + sizeof(value))
   */
  ASSERT (nbytes > sizeof (pgno));

  p_size ret = (nbytes - sizeof (pgno)) / (sizeof (pgno) + sizeof (p_size));

  ASSERT (ret > 0);

  return ret;
}

void
in_clip_right (inner_node *in, p_size fromkey)
{
  valid_inner_node_assert (in);
  ASSERT (fromkey <= *in->nkeys);

  if (fromkey == *in->nkeys)
    {
      return;
    }

  p_size nclip = *in->nkeys - fromkey;
  *in->nkeys -= nclip;
  *in->keys += nclip;
}

p_size
in_keys_avail (inner_node *in)
{
  valid_inner_node_assert (in);

  p_size maxkeys = in_max_nkeys (in->rlen);

  ASSERT (maxkeys >= *in->nkeys);

  return maxkeys - *in->nkeys;
}

p_size
in_get_nkeys (const inner_node *in)
{
  valid_inner_node_assert (in);

  return *in->nkeys;
}

b_size
in_get_key (inner_node *in, p_size idx)
{
  valid_inner_node_assert (in);

  ASSERT (idx < *in->nkeys);

  b_size key = in->keys[*in->nkeys - 1 - idx];

  return key;
}

pgno
in_get_leaf (inner_node *in, p_size idx)
{
  valid_inner_node_assert (in);

  ASSERT (idx < *in->nkeys);

  pgno leaf = in->leafs[idx];

  return leaf;
}

pgno
in_get_right_most_leaf (inner_node *in)
{
  valid_inner_node_assert (in);

  return in->leafs[*in->nkeys];
}
