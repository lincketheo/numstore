#include "ns_testing.h"

int
main ()
{
  for (int i = 0; i < test_count; ++i)
    {
      tests[i]();
    }
  return 0;
}
