#include "paging/page.h"
#include "dev/assert.h"
#include "dev/errors.h"
#include "intf/stdlib.h"
#include "paging/types/data_list.h"
#include "utils/bounds.h"

///////////////////////////// PAGE TYPES

#define MIN_PAGE_SIZE 512

DEFINE_DBG_ASSERT_I (page, page, p)
{
  ASSERT (p);
}

static inline void
page_set_ptrs (page *p, u8 *raw, p_size rlen, page_type type, pgno pg)
{
  ASSERT (p);
  ASSERT (raw);

  switch (type)
    {
    case PG_DATA_LIST:
      {
        p->dl = dl_set_ptrs (raw, rlen);
        break;
      }
    default:
      {
        panic ();
      }
    }

  p->type = type;
  p->pg = pg;
}

err_t
page_read_expect (page *dest, int type, u8 *raw, p_size rlen, pgno pg)
{
  ASSERT (dest);

  // Check that this page is
  // the page type we want
  ASSERT (rlen > 0);
  u8 header = raw[0];
  if (!(header & type))
    {
      return ERR_INVALID_STATE;
    }

  page_set_ptrs (dest, raw, rlen, (page_type)header, pg);

  return SUCCESS;
}

void
page_init (page *dest, page_type type, u8 *raw, p_size rlen, pgno pg)
{
  ASSERT (dest);

  page_set_ptrs (dest, raw, rlen, type, pg);

  // Aditional Setup
  switch (type)
    {
    case PG_DATA_LIST:
      {
        dl_init_empty (&dest->dl);
        break;
      }
    default:
      {
        panic ();
        break;
      }
    }
}
