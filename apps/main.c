#include "config.h"
#include "os/io.h"
#include "os/stdlib.h"
#include "paging/pager.h"

int
main (void)
{
  pager p;
  i_memset (&p, 0, sizeof p);
  string foo = {
    .data = "foo",
    .len = 3,
  };
  i_file *fd = i_open (&foo, 1, 1);
  p.fpager.f = fd;

  int ret;
  u64 ptr;
  if ((ret = pgr_new (&p, &ptr)))
    {
      return ret;
    }

  u8 *page;

  if ((ret = pgr_get (&p, &page, 0)))
    {
      return ret;
    }

  memcpy (page, "Hello", 5);
  if ((ret = pgr_commit (&p, 0)))
    {
      return ret;
    }

  i_close (fd);

  return 0;
}
