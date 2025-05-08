#include "paging/page.h"
#include "dev/assert.h"
#include "dev/errors.h"
#include "intf/stdlib.h"
#include "paging/types/data_list.h"
#include "paging/types/hash_leaf.h"
#include "paging/types/hash_page.h"
#include "paging/types/inner_node.h"
#include "utils/bounds.h"

///////////////////////////// PAGE TYPES

#define MIN_PAGE_SIZE 512

DEFINE_DBG_ASSERT_I (page, page, p)
{
  ASSERT (p);
}

err_t
page_read_expect (page *dest, int type, u8 *raw, p_size rlen, pgno pg)
{
  ASSERT (dest);
  ASSERT (raw);
  ASSERT (rlen > 0);

  err_t ret = SUCCESS;

  /**
   * Check that the header matches
   */
  u8 header = raw[0];
  if (!(header & type))
    {
      return ERR_INVALID_STATE;
    }

  switch (type)
    {
    case PG_DATA_LIST:
      {
        data_list dl = dl_set_ptrs (raw, rlen);
        if (!dl_is_valid (&dl))
          {
            return ERR_INVALID_STATE;
          }
        dest->dl = dl;
        break;
      }
    case PG_INNER_NODE:
      {
        if ((ret = in_read_and_set_ptrs (&dest->in, raw, rlen)))
          {
            return ret;
          }
        break;
      }
    case PG_HASH_PAGE:
      {
        hash_page hp = hp_set_ptrs (raw, rlen);
        if (!hp_is_valid (&hp))
          {
            return ERR_INVALID_STATE;
          }
        dest->hp = hp;
        break;
      }
    case PG_HASH_LEAF:
      {
        hash_leaf hl = hl_set_ptrs (raw, rlen);
        if (!hl_is_valid (&hl))
          {
            return ERR_INVALID_STATE;
          }
        dest->hl = hl;
        break;
      }
    default:
      {
        panic ();
      }
    }

  dest->type = type;
  dest->pg = pg;

  return SUCCESS;
}

void
page_init (page *dest, page_type type, u8 *raw, p_size rlen, pgno pg)
{
  ASSERT (dest);

  dest->type = type;
  dest->pg = pg;

  switch (type)
    {
    case PG_DATA_LIST:
      {
        dest->dl = dl_set_ptrs (raw, rlen);
        dl_init_empty (&dest->dl);
        break;
      }
    case PG_INNER_NODE:
      {
        dest->in = in_set_initial_ptrs (raw, rlen);
        in_init_empty (&dest->in);
        break;
      }
    case PG_HASH_PAGE:
      {
        dest->hp = hp_set_ptrs (raw, rlen);
        hp_init_empty (&dest->hp);
        break;
      }
    case PG_HASH_LEAF:
      {
        dest->hl = hl_set_ptrs (raw, rlen);
        hl_init_empty (&dest->hl);
        break;
      }
    default:
      {
        panic ();
      }
    }
}
