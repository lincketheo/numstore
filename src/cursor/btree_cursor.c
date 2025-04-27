#include "cursor/btree_cursor.h"
#include "dev/assert.h"
#include "dev/errors.h"
#include "paging/page.h"
#include "paging/pager.h"
#include "paging/types/data_list.h"
#include "paging/types/hash_leaf.h"
#include "paging/types/inner_node.h"
#include "utils/bounds.h"

DEFINE_DBG_ASSERT_I (btree_cursor, btree_cursor, b)
{
  ASSERT (b);
}

void
btc_create (btree_cursor *dest, btc_params params)
{
  dest->is_page_loaded = false;
  dest->pager = params.pager;
  dest->stack_allocator = params.stack_allocator;
  btree_cursor_assert (dest);
}

// Open / close
err_t
btc_open (btree_cursor *dest, u64 pg0)
{
  btree_cursor_assert (dest);
  ASSERT (!dest->is_page_loaded);

  err_t ret = SUCCESS;

  page root;
  if ((ret = pgr_get_expect (&root, PG_INNER_NODE, pg0, dest->pager)))
    {
      return ret;
    }

  dest->root = root;
  dest->is_page_loaded = true;
  dest->is_seeked = false;

  return SUCCESS;
}

err_t
btc_close (btree_cursor *n)
{
  btree_cursor_assert (n);
  ASSERT (n->is_page_loaded);
  n->is_page_loaded = false;
  return SUCCESS;
}

err_t
_btc_seek_recurse (btree_cursor *n, u64 loc)
{
  page cur = n->cur_page;
  switch (*cur.header)
    {
    case (u8)PG_INNER_NODE:
      {
        u64 before;
        u64 next_page = in_choose_leaf (&cur.in, &before, loc);

        ASSERT (before <= loc);

        loc -= before;

        // Get next page:
        err_t ret = pgr_get_expect (
            &n->cur_page,
            PG_INNER_NODE | PG_DATA_LIST,
            next_page, n->pager);
        if (ret)
          {
            return ret;
          }

        return _btc_seek_recurse (n, loc);
      }
    case (u8)PG_DATA_LIST:
      {
        data_list dl = n->cur_page.dl;
        if (loc > *dl.blen)
          {
            ASSERT (0); // TODO
          }
        n->is_seeked = true;
        n->idx = loc;
        return SUCCESS;
      }
    default:
      {
        return ERR_INVALID_STATE;
      }
    }
}

err_t
btc_seek (btree_cursor *n, u64 size, u64 *nelem, seek_t whence)
{
  btree_cursor_assert (n);
  ASSERT (n->is_page_loaded);
  ASSERT (!n->is_seeked);
  ASSERT (size > 0);
  ASSERT (nelem);
  ASSERT (whence);

  if (!can_mul_u16 (*nelem, size))
    {
      return ERR_ARITH;
    }
  u64 byte = *nelem * size;

  // Find the start
  ASSERT (*n->root.header == PG_INNER_NODE);

  return _btc_seek_recurse (n, byte);
}
