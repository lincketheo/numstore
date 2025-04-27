#include "paging/types/inner_node.h"
#include "dev/assert.h"

DEFINE_DBG_ASSERT_I (inner_node, inner_node, i)
{
  ASSERT (i);
  ASSERT (i->nkeys);
  ASSERT (i->leafs);
  ASSERT (i->keys);
}

u64
in_choose_leaf (const inner_node *node, u64 *before, u64 loc)
{
  inner_node_assert (node);
  ASSERT (before);

  u32 n = in_get_nkeys (node);
  u32 i = 0;

  for (; i < n; ++i)
    {
      if (loc <= in_get_key (node, i))
        {
          break;
        }
    }

  if (i == 0)
    {
      *before = 0;
    }
  else
    {
      *before = in_get_key (node, i - 1);
    }

  return in_get_leaf (node, i);
}

u64
in_get_key (const inner_node *node, u32 idx)
{
  inner_node_assert (node);
  ASSERT (idx <= *node->nkeys);
  return node->keys[*node->nkeys - idx];
}

u64
in_get_leaf (const inner_node *node, u32 idx)
{
  inner_node_assert (node);
  ASSERT (idx <= (*node->nkeys + 1));
  return node->leafs[idx];
}
