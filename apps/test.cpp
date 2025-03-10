#include "testing.hpp"

int
main ()
{
  for (int i = 0; i < ntests; ++i)
    {
      tests[i]();
    }

  return test_ret;
}
