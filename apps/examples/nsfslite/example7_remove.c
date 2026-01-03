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
 * Example 7: Remove Contiguous Data
 *
 * Demonstrates removing a contiguous range of data from the middle of a dataset.
 * Shows how nsfslite handles deletion of elements and compacts the remaining data.
 *
 * Usage:
 *   ./example7_remove
 */

#include "nsfslite.h"
#include "utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define N_ELEMS 200000
#define REMOVE_OFFSET 50000
#define REMOVE_COUNT 100000

int
main (void)
{
  int ret = 0;
  nsfslite *n = NULL;
  int *data = malloc (N_ELEMS * sizeof (int));
  int *read_data = malloc ((N_ELEMS - REMOVE_COUNT) * sizeof (int));
  int *expected = malloc ((N_ELEMS - REMOVE_COUNT) * sizeof (int));

  for (size_t i = 0; i < N_ELEMS; i++)
    {
      data[i] = i;
    }

  for (size_t i = 0; i < REMOVE_OFFSET; i++)
    {
      expected[i] = i;
    }
  for (size_t i = REMOVE_OFFSET; i < N_ELEMS - REMOVE_COUNT; i++)
    {
      expected[i] = i + REMOVE_COUNT;
    }

  unlink ("test7.db");
  unlink ("test7.wal");

  n = nsfslite_open ("test7.db", "test7.wal");
  if (!n)
    {
      fprintf (stderr, "Failed to open database\n");
      ret = -1;
      goto cleanup;
    }

  int64_t id = nsfslite_new (n, NULL, "data");
  CHECK (id);
  CHECK (nsfslite_insert (n, id, NULL, data, 0, sizeof (int), N_ELEMS));

  struct nsfslite_stride rstride_remove = { .bstart = REMOVE_OFFSET * sizeof (int), .stride = 1, .nelems = REMOVE_COUNT };
  CHECK (nsfslite_remove (n, id, NULL, NULL, sizeof (int), rstride_remove));

  nsfslite_close (n);

  n = nsfslite_open ("test7.db", "test7.wal");
  if (!n)
    {
      fprintf (stderr, "Failed to reopen database\n");
      ret = -1;
      goto cleanup;
    }

  id = nsfslite_get_id (n, "data");
  CHECK (id);

  struct nsfslite_stride rstride = { .bstart = 0, .stride = 1, .nelems = N_ELEMS - REMOVE_COUNT };
  CHECK (nsfslite_read (n, id, read_data, sizeof (int), rstride));

  for (size_t i = 0; i < N_ELEMS - REMOVE_COUNT; i++)
    {
      if (expected[i] != read_data[i])
        {
          fprintf (stderr, "Mismatch at %zu: expected %d, got %d\n", i, expected[i], read_data[i]);
          ret = -1;
          goto cleanup;
        }
    }

  printf ("SUCCESS: Removed %d elements, verified %d remaining after reopen\n", REMOVE_COUNT, N_ELEMS - REMOVE_COUNT);

cleanup:
  if (n)
    nsfslite_close (n);
  free (data);
  free (read_data);
  free (expected);
  return ret;
}
