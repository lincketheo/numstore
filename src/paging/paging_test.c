#include "config.h"
#include "dev/testing.h"
#include "paging/memory_pager.h"
#include "paging/pager.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static void
gen_buf (u64 seed, u8 *buf, size_t n)
{
  u64 st = seed;
  for (u64 i = 0; i < n; i++)
    {
      st = st * 6364136223846793005ULL + 1;
      buf[i] = (u8)(st >> 32);
    }
}

#define NPAGES 10000
#define NACCESS 100000

TEST (paging)
{
  // Remove file (might error, that's ok)
  test_fail_if (i_remove_quiet (cstrfrom ("test_test")));

  // Create the pager
  pager p;
  i_memset (&p, 0, sizeof p);

  i_file *fd = i_open (cstrfrom ("test_file"), 1, 1);
  test_fail_if_null (fd);

  p.fpager.f = fd;

  /**
   * Do two passes.
   *
   * 1. For each iter until you've written 1000 pages,
   * choose between ARW or RT
   *    ARW - Allocate new page, read that page, write random , commit
   *    RT - Read any prev page, test it's bytes
   * 2. For 10000 iters, choose between RW or RT
   *    RW - Read a random page, write random to it, commit
   *    RT - Same as above, but any page 0, 1000
   */

  // For this test, ensure there's at least
  // enough to overflow the memory buffer
  test_fail_if (BPLENGTH > NPAGES);

  u64 pgn[NPAGES], s[NPAGES];
  u8 *pg;
  u8 tmp[PAGE_SIZE];

  int i, j, op, n = 0;

  srand ((unsigned int)time (NULL));

  // Pass 1 - ARW or RT
  while (n < NPAGES)
    {
      op = rand () % 2;

      if (op == 0 || n == 0)
        {
          // Allocate, Read, Write, Commit
          u64 seed = (((u64)rand () << 32) | rand ());
          test_fail_if (pgr_new (&p, &pgn[n]));
          test_fail_if (pgr_get (&p, &pg, pgn[n]));
          gen_buf (seed, pg, PAGE_SIZE);
          test_fail_if (pgr_commit (&p, pgn[n]));
          s[n++] = seed;
        }
      else
        {
          // Read, Test
          j = rand () % n;
          test_fail_if (pgr_get (&p, &pg, pgn[j]));

          // Generate the same random buffer
          gen_buf (s[j], tmp, PAGE_SIZE);
          test_assert_int_equal (memcmp (pg, tmp, PAGE_SIZE), 0);
        }
    }

  // Pass 2 - RW or RT
  for (i = 0; i < NACCESS; i++)
    {
      op = rand () % 2;
      j = rand () % NPAGES;

      if (op == 0)
        {
          // Read, Write, Commit
          test_fail_if (pgr_get (&p, &pg, pgn[j]));
          u64 seed = (((u64)rand () << 32) | rand ());
          gen_buf (seed, pg, PAGE_SIZE);
          test_fail_if (pgr_commit (&p, pgn[j]));
          s[j] = seed;
        }
      else
        {
          // Read, Test
          test_fail_if (pgr_get (&p, &pg, pgn[j]));
          gen_buf (s[j], tmp, PAGE_SIZE);
          test_assert_int_equal (memcmp (pg, tmp, PAGE_SIZE), 0);
        }
    }
}
