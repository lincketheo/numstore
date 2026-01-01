/*
 * Copyright (c) 2025 Theo Lincke
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Example 1: Basic Persistence
 *
 * Demonstrates basic persistence and data recovery in nsfslite. Creates a database,
 * inserts data, closes and reopens the database, then verifies the data was
 * persisted correctly.
 *
 * Usage:
 *   ./example1_basic_persistence
 */

#include "nsfslite.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#define N_ELEMS 200000

static inline long
elapsed_ms (struct timespec s, struct timespec e)
{
  return (e.tv_sec - s.tv_sec) * 1000 + (e.tv_nsec - s.tv_nsec) / 1000000;
}

int
main (void)
{
  int ret = 0;
  nsfslite *n = NULL;
  int *data = malloc (N_ELEMS * sizeof (int));
  int *read_data = malloc (N_ELEMS * sizeof (int));

  for (size_t i = 0; i < N_ELEMS; i++)
    {
      data[i] = i;
    }

  unlink ("test1.db");
  unlink ("test1.wal");

  n = nsfslite_open ("test1.db", "test1.wal");
  if (!n)
    {
      fprintf (stderr, "Failed to open database\n");
      ret = -1;
      goto cleanup;
    }

  int64_t id = nsfslite_new (n, NULL, "data");
  if (id < 0)
    {
      fprintf (stderr, "Failed to create variable: %s\n", nsfslite_error (n));
      ret = -1;
      goto cleanup;
    }

  struct timespec st, en;
  clock_gettime (CLOCK_MONOTONIC, &st);

  if (nsfslite_insert (n, id, NULL, data, 0, sizeof (int), N_ELEMS) < 0)
    {
      fprintf (stderr, "Failed to insert: %s\n", nsfslite_error (n));
      ret = -1;
      goto cleanup;
    }

  clock_gettime (CLOCK_MONOTONIC, &en);
  printf ("Time: %ld ms\n", elapsed_ms (st, en));

  nsfslite_close (n);

  n = nsfslite_open ("test1.db", "test1.wal");
  if (!n)
    {
      fprintf (stderr, "Failed to reopen database\n");
      ret = -1;
      goto cleanup;
    }

  if (nsfslite_read (n, id, read_data, sizeof (int),
                     (struct nsfslite_stride){ .bstart = 0, .stride = 1, .nelems = N_ELEMS })
      < 0)
    {
      fprintf (stderr, "Failed to read: %s\n", nsfslite_error (n));
      ret = -1;
      goto cleanup;
    }

  for (size_t i = 0; i < N_ELEMS; i++)
    {
      if (data[i] != read_data[i])
        {
          fprintf (stderr, "Mismatch at %zu: expected %d, got %d\n", i, data[i], read_data[i]);
          ret = -1;
          goto cleanup;
        }
    }

  printf ("SUCCESS: All %d elements match after reopen\n", N_ELEMS);

cleanup:
  if (n)
    nsfslite_close (n);
  free (data);
  free (read_data);
  return ret;
}
