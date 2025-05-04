#include "paging/types/inner_node.h"
#include "dev/assert.h"

DEFINE_DBG_ASSERT_I (inner_node, inner_node, i)
{
  ASSERT (i);
  ASSERT (i->nkeys);
  ASSERT (i->leafs);
  ASSERT (i->keys);
  ASSERT (*i->nkeys > 0);
  // TODO - upper bounds on nkeys
}

pgno
in_choose_leaf (const inner_node *node, b_size *left, b_size loc)
{
  inner_node_assert (node);
  ASSERT (left);

  u32 n = *node->nkeys;
  const b_size *keys = node->keys;
  const pgno *leafs = node->leafs;

  b_size prev_key = 0;

  for (u32 i = 0; i < n; ++i)
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
          *left = prev_key;
          return leafs[i];
        }

      // advance prev_key for the next iteration
      prev_key = key;
    }

  /**
   *                 [5]
   * [0, 1, 2, 3, 4]    [5, 6, 7, 8, 9]
   *
   * For loc == 2, left = 0 (initial prev_key)
   * For loc == 8, left = 5 (last prev_key)
   */
  *left = prev_key;
  return leafs[n];
}
