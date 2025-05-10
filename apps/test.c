#include "dev/testing.h"
#include "intf/io.h"

int
main (void)
{
  for (u64 i = 0; i < ntests; ++i)
    {
      tests[i]();
    }

  return test_ret;
}
