#include "core/dev/testing.h"
#include "core/intf/io.h"
#include "core/intf/stdlib.h"
#include "core/utils/macros.h"

#include <stdio.h>
#include <time.h>

static inline long
elapsed_ms (struct timespec start, struct timespec end)
{
  return (end.tv_sec - start.tv_sec) * 1000
         + (end.tv_nsec - start.tv_nsec) / 1000000;
}

int
main (int argc, char **argv)
{
  struct timespec start, end;
  clock_gettime (CLOCK_MONOTONIC, &start);

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

  clock_gettime (CLOCK_MONOTONIC, &end);
  printf ("Time: %ld ms\n", elapsed_ms (start, end));

  return test_ret;
}
