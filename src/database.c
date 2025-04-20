#include "database.h"
#include "dev/errors.h"
#include "intf/io.h"
#include "intf/logging.h"
#include "intf/mm.h"
#include "paging.h"
#include "sds.h"
#include "types.h"

static global_config c;

const global_config *
get_global_config ()
{
  return &c;
}

void
set_global_config (u32 page_size, u32 mpgr_len)
{
  c.header_size = sizeof (u32) * 2;
  c.page_size = page_size;
  c.mpgr_len = mpgr_len;
}

err_t
db_create (const string fname, u32 page_size, u32 mpgr_len)
{
  string_assert (&fname);
  set_global_config (page_size, mpgr_len);

  u8 *hash_page = NULL; // A temp page for writing header etc
  i_file *fp = NULL;
  int ret = SUCCESS;

  // Check if database already exists
  if (i_exists_rw (fname))
    {
      i_log_warn ("Trying to create a new database: %.*s, "
                  "but it already exists. Try opening it instead\n",
                  fname.len, fname.data);
      ret = ERR_ALREADY_EXISTS;
      goto theend;
    }

  fp = i_open (fname, 1, 1);

  // Serialize header
  u32 header[2] = { page_size, mpgr_len };
  ret = i_write_all (fp, header, sizeof (header), 0);
  if (ret)
    {
      i_log_warn ("Failed to write header on database create\n");
      goto theend;
    }

  // Wrap file in pager
  file_pager fpgr;
  fpgr_create (&fpgr, fp);

  // Allocate hash page
  u64 pgno;
  ret = fpgr_new (&fpgr, &pgno);
  if (ret)
    {
      i_log_warn ("Failed to allocate "
                  "first page to database\n");
      goto theend;
    }
  // TODO - is there any case this can't be 0? Race Condition?
  ASSERT (pgno == 0);

  // Create Hash Page
  hash_page = i_malloc (page_size);
  if (hash_page == NULL)
    {
      i_log_warn ("Failed to allocate memory for hash "
                  "page in db_create\n");
      ret = ERR_IO;
      goto theend;
    }

  // Initialize Hash Page
  page p;
  page_init (&p, PG_HASH_PAGE, hash_page);

  // Write hash page to disk
  ret = fpgr_commit (&fpgr, hash_page, pgno);
  if (ret)
    {
      i_log_warn ("Failed to commit "
                  "first hash page to database\n");
      goto theend;
    }

theend:
  if (hash_page)
    {
      i_free (hash_page);
    }
  if (fp)
    {
      i_close (fp);
    }
  return ret;
}
