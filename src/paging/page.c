#include "paging/page.h"
#include "dev/assert.h"
#include "dev/errors.h"
#include "intf/stdlib.h"
#include "paging/types/data_list.h"
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

  dest->type = type;
  dest->pg = pg;

  switch (type)
    {
    case PG_DATA_LIST:
      {
        dest->dl = dl_set_ptrs (raw, rlen);
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
    default:
      {
        panic ();
      }
    }

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
    default:
      {
        panic ();
      }
    }
}
