#pragma once

#include "common/types.h"
#include "os/io.h"
#include "paging/pager.h"
#include "paging/types/data_list.h"
#include "paging/types/index_list.h"

typedef enum
{
  PG_DATA_LIST,
  PG_INDEX_LIST,
} pg_type;

typedef union
{
  data_list dl;
  index_list il;
} any_page;

static inline int
pgr_get_typed (pager *p, any_page *dest, u64 ptr, pg_type t)
{
  u8 *pg;

  int ret = pgr_get (p, &pg, ptr);
  if (ret)
    {
      i_log_warn ("Failed to get page from pgr_get_typed\n");
      return ret;
    }

  switch (t)
    {
    case PG_DATA_LIST:
      {
        return dl_read_page (&dest->dl, pg);
      }
    case PG_INDEX_LIST:
      {
        return il_read_page (&dest->il, pg);
      }
    default:
      ASSERT (0);
    }
}
