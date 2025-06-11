#include "paging/types/inner_node.h"

#include "config.h"
#include "dev/assert.h"  // DEFINE_DBG_ASSERT_I
#include "paging/page.h" // PG_INNER_NODE

DEFINE_DBG_ASSERT_I (inner_node, unchecked_inner_node, i)
{
  ASSERT (i);
}

DEFINE_DBG_ASSERT_I (inner_node, valid_inner_node, i)
{
  error e = error_create (NULL);
  ASSERT (in_validate (i, &e) == SUCCESS);
}

err_t
in_validate (const inner_node *in, error *e)
{
  unchecked_inner_node_assert (in);

  if (*in->header != (pgh)PG_INNER_NODE)
    {
      return error_causef (
          e, ERR_CORRUPT,
          "Inner Node: expected header: %" PRpgh " but got: %" PRpgh,
          (pgh)PG_INNER_NODE, *in->header);
    }

  /**
   * Check that nkeys is less than max keys
   */
  if (*in->nkeys > IN_MAX_KEYS)
    {
      return error_causef (
          e, ERR_CORRUPT,
          "Inner Node: nkeys (%" PRp_size ") is more than "
          "maximum nkeys (%" PRp_size ") for page size: %" PRp_size,
          *in->nkeys, IN_MAX_KEYS, PAGE_SIZE);
    }

  /**
   * Inner nodes must contain at least
   * 1 key
   */
  if (*in->nkeys == 0)
    {
      return error_causef (
          e, ERR_CORRUPT,
          "Inner Node: nkeys cannot be 0");
    }

  if (in->keys == NULL)
    {
      return error_causef (
          e, ERR_CORRUPT,
          "Inner Node: nkeys > 0, but there are no keys");
    }

  return SUCCESS;
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

void
in_set_initial_ptrs (inner_node *in, u8 raw[PAGE_SIZE])
{
  *in = (inner_node){
    .header = (pgh *)&raw[IN_HEDR_OFST],
    .nkeys = (p_size *)&raw[IN_NKEY_OFST],
    .leafs = (pgno *)&raw[IN_LEAF_OFST],
    .keys = NULL,
  };
  unchecked_inner_node_assert (in);
  // We are in an INVALID state because we haven't read anything yet
}

void
in_init_empty (inner_node *in)
{
  ASSERT (in);
  *in->header = PG_INNER_NODE;
  *in->nkeys = 0;
}

err_t
in_read_and_set_ptrs (inner_node *dest, u8 raw[PAGE_SIZE], error *e)
{
  // First set pointers to initial positions
  in_set_initial_ptrs (dest, raw);

  // Then adjust keys
  if (*dest->nkeys > IN_MAX_KEYS)
    {
      return error_causef (
          e, ERR_CORRUPT,
          "Inner Node: nkeys (%u) is more than "
          "maximum nkeys (%u) for page size: %u",
          *dest->nkeys, (p_size)IN_MAX_KEYS, PAGE_SIZE);
    }

  dest->keys = (b_size *)(&raw[PAGE_SIZE - *dest->nkeys]);

  err_t_wrap (in_validate (dest, e), e);

  valid_inner_node_assert (dest);

  return SUCCESS;
}

void
in_init (
    inner_node *dest,
    b_size key,
    pgno left,
    pgno right)
{
  unchecked_inner_node_assert (dest);
  *dest->header = PG_INNER_NODE;
  dest->leafs[0] = left;
  dest->leafs[1] = right;

  dest->keys = (b_size *)((u8 *)dest->leafs + PAGE_SIZE - IN_LEAF_OFST) - 1;
  dest->keys[0] = key;
  *dest->nkeys = 1;
  valid_inner_node_assert (dest);
}

bool
in_add_kv (inner_node *dest, b_size key, pgno right)
{
  valid_inner_node_assert (dest);

  if (*dest->nkeys == IN_MAX_KEYS)
    {
      return false;
    }

  // Grow leafs to the right
  dest->leafs[++(*dest->nkeys)] = right;

  // Grow keys to the left
  dest->keys -= 1;
  dest->keys[0] = key;

  return true;
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
in_keys_avail (const inner_node *in)
{
  unchecked_inner_node_assert (in);

  p_size maxkeys = IN_MAX_KEYS;

  ASSERT (maxkeys >= *in->nkeys);

  return maxkeys - *in->nkeys;
}

p_size
in_get_nkeys (const inner_node *in)
{
  unchecked_inner_node_assert (in);

  return *in->nkeys;
}

b_size
in_get_key (const inner_node *in, p_size idx)
{
  valid_inner_node_assert (in);

  ASSERT (idx < *in->nkeys);

  b_size key = in->keys[*in->nkeys - 1 - idx];

  return key;
}

pgno
in_get_leaf (const inner_node *in, p_size idx)
{
  valid_inner_node_assert (in);

  ASSERT (idx < *in->nkeys);

  pgno leaf = in->leafs[idx];

  return leaf;
}

pgno
in_get_right_most_leaf (const inner_node *in)
{
  valid_inner_node_assert (in);

  return in->leafs[*in->nkeys];
}

b_size
in_get_right_most_key (const inner_node *in)
{
  valid_inner_node_assert (in);

  return in->keys[0];
}

void
i_log_in (const inner_node *in)
{
  valid_inner_node_assert (in);

  i_log_info ("=== INNER NODE PAGE START ===\n");

  i_log_info ("HEADER : %" PRpgh "\n", *in->header);
  i_log_info ("NKEYS  : %u\n", *in->nkeys);

  // Log leaf pointers
  for (u32 i = 0; i <= *in->nkeys; ++i)
    {
      i_log_info ("LEAF[%u] = %" PRpgno "\n", i, in_get_leaf (in, i));
    }

  // Log keys in forward logical order (which is reverse in memory)
  for (u32 i = 0; i < *in->nkeys; ++i)
    {
      u32 rev = (*in->nkeys - 1) - i;
      i_log_info ("KEY[%u]  = %" PRb_size "\n", i, in_get_key (in, rev));
    }

  i_log_info ("=== INNER NODE PAGE END ===\n");
}
