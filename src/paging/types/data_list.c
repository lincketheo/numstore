#include "paging/types/data_list.h"
#include "common/macros.h"
#include "common/types.h"
#include "config.h"
#include "dev/errors.h"
#include "os/io.h"
#include "os/stdlib.h"
#include "paging/types.h"

DEFINE_DBG_ASSERT (data_list, data_list, d)
{
  ASSERT (d);
  ASSERT (d->header);
  ASSERT (d->next);
  ASSERT (d->idx_num);
  ASSERT (d->idx_den);
  ASSERT (d->data);
}

private
inline int
dl_valid_page_type (u8 *page)
{
  ASSERT (page);
  u8 type = page[0];
  return type == (u8)PG_DATA_LIST;
}

private
inline void
dl_assign_ptrs (data_list *dest, u8 *page)
{
  dest->page = page;

  int idx = 0;
  dest->header = &page[idx];
  idx += sizeof (dest->header);

  dest->next = (u64 *)(&page[idx]);
  idx += sizeof (dest->next);

  dest->idx_num = (u64 *)&page[idx];
  idx += sizeof (dest->idx_num);

  dest->idx_den = (u64 *)&page[idx];
  idx += sizeof (dest->idx_den);

  dest->len = (u64 *)&page[idx];
  idx += sizeof (dest->len);

  dest->data = &page[idx];
}

int
dl_read_page (data_list *dest, u8 *page)
{
  ASSERT (dest);
  ASSERT (page);
  if (!dl_valid_page_type (page))
    {
      i_log_error ("Expected data list page type. Data is malformed\n");
      return ERR_INVALID_STATE;
    }

  dl_assign_ptrs (dest, page);

  return SUCCESS;
}

void
dl_read_and_init_page (data_list *dest, u8 *page)
{
  ASSERT (dest);
  ASSERT (page);

  dl_assign_ptrs (dest, page);
  i_memset (dest->page, 0, PAGE_SIZE);

  *dest->header = (u8)PG_DATA_LIST;
}
