#include "dev/testing.h"
#include "os/io.h"

int
main (void)
{
  for (u64 i = 0; i < ntests; ++i)
    {
      tests[i]();
    }

  io_log_stats ();

  return test_ret;
}
