#include "dev/testing.h"
#include "os/io.h"
#include "paging/pager.h"

int
main (void)
{
  for (u64 i = 0; i < ntests; ++i)
    {
      tests[i]();
    }

  io_log_stats ();
  pgr_log_stats ();

  return test_ret;
}
