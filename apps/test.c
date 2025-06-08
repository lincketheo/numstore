#include "dev/testing.h"
#include "intf/io.h"
#include "intf/stdlib.h"
#include "utils/macros.h"

int
main (int argc, char **argv)
{
  if (argc > 1)
    {
      for (int arg = 0; arg < argc; ++arg)
        {
          u32 alen = i_unsafe_strlen (argv[arg]);

          for (u64 t = 0; t < ntests; ++t)
            {
              if (i_strncmp (argv[arg], tests[t].test_name, MIN (alen, tests[t].nlen)) == 0)
                {
                  test_func func = tests[t].test;
                  func ();
                  break;
                }
            }
        }
    }
  else
    {
      for (u64 i = 0; i < ntests; ++i)
        {
          test_func func = tests[i].test;
          func ();
        }
    }

  return test_ret;
}
