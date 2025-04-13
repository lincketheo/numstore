#include "paging/file_pager.h"
#include "common/types.h"
#include "config.h"
#include "dev/errors.h"
#include "os/io.h"

int
fpgr_new (file_pager *p, u64 *dest)
{
  file_pager_assert (p);
  ASSERT (dest);

  // Get the file size
  i64 size = i_file_size (p->f);
  if (size < 0)
    {
      return (int)size;
    }

  // Check if valid file state
  if (size % PAGE_SIZE != 0)
    {
      i_log_error ("Noticed the database is in an invalid state "
                   "when trying to create a new page. File size "
                   "is not a multiple of page size: %zu bytes\n",
                   PAGE_SIZE);
      return ERR_INVALID_STATE;
    }

  u64 newsize = (u64)size + PAGE_SIZE;
  int ret = i_truncate (p->f, newsize);
  if (ret)
    {
      i_log_warn ("Error truncating while trying to create new file\n");
      return ret;
    }

  *dest = size / PAGE_SIZE;

  return SUCCESS;
}

int
fpgr_delete (file_pager *p, u64 ptr)
{
  // TODO
  file_pager_assert (p);
  ASSERT (ptr > 0);
  return SUCCESS;
}

int
fpgr_get (file_pager *p, u8 dest[PAGE_SIZE], u64 ptr)
{
  file_pager_assert (p);
  ASSERT (dest);

  i64 nread = i_read_all (p->f, dest, PAGE_SIZE, ptr * PAGE_SIZE);

  if (nread == 0)
    {
      i_log_error ("Early EOF encountered on fpgr_get\n");
      return ERR_INVALID_STATE;
    }

  if (nread < 0)
    {
      i_log_warn ("Invalid read encountered on fpgr_get\n");
      return ERR_IO;
    }

  return SUCCESS;
}

int
fpgr_commit (file_pager *p, const u8 src[PAGE_SIZE], u64 ptr)
{
  file_pager_assert (p);
  ASSERT (src);

  int ret = i_write_all (p->f, src, PAGE_SIZE, ptr * PAGE_SIZE);
  if (ret)
    {
      i_log_warn ("Write failure happened while trying to commit\n");
      return ret;
    }

  return SUCCESS;
}
