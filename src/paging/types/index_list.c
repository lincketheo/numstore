#include "paging/types/index_list.h"
#include "common/macros.h"
#include "dev/errors.h"
#include "os/io.h"
#include "paging/types.h"

DEFINE_DBG_ASSERT (index_list, index_list, i)
{
  ASSERT (i);
  ASSERT (i->header);
  ASSERT (i->next);
  ASSERT (i->size);
  ASSERT (i->data);
}

private
inline int
il_valid_page_type (u8 *page)
{
  ASSERT (page);
  u8 type = page[0];
  return type == (u8)PG_INDEX_LIST;
}

int
il_read_page (index_list *dest, u8 *page)
{
  ASSERT (dest);
  ASSERT (page);

  if (!il_valid_page_type (page))
    {
      i_log_error ("Expected index list page type. Data is malformed\n");
      return ERR_INVALID_STATE;
    }

  int idx = 0;
  dest->header = &page[idx];
  idx += sizeof (dest->header);

  dest->next = (u64 *)(&page[idx]);
  idx += sizeof (dest->next);

  dest->size = &page[idx];
  idx += sizeof (dest->size);

  dest->len = &page[idx];
  idx += sizeof (dest->len);

  dest->data = &page[idx];

  return SUCCESS;
}
