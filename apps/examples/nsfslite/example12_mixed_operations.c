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
 * Example 12: Mixed Operations
 *
 * Demonstrates a combination of insert, write, and remove operations on the same
 * dataset. Shows how different operations can be composed to perform complex data
 * transformations.
 *
 * Usage:
 *   ./example12_mixed_operations
 */

#include "nsfslite.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define N_ELEMS 200000

int
main (void)
{
  int ret = 0;
  nsfslite *n = NULL;
  int *data = malloc (N_ELEMS * sizeof (int));
  int *read_data = malloc (N_ELEMS * sizeof (int));
  int *update = malloc (10000 * sizeof (int));

  for (size_t i = 0; i < N_ELEMS; i++)
    {
      data[i] = i;
    }

  unlink ("test12.db");
  unlink ("test12.wal");

  n = nsfslite_open ("test12.db", "test12.wal");
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

  if (nsfslite_insert (n, id, NULL, data, 0, sizeof (int), N_ELEMS) < 0)
    {
      fprintf (stderr, "Failed to insert: %s\n", nsfslite_error (n));
      ret = -1;
      goto cleanup;
    }

  for (size_t i = 0; i < 10000; i++)
    {
      update[i] = 55000 + i;
    }

  if (nsfslite_write (n, id, NULL, update, sizeof (int),
                      (struct nsfslite_stride){ .bstart = 50000 * sizeof (int), .stride = 1, .nelems = 10000 })
      < 0)
    {
      fprintf (stderr, "Failed to write: %s\n", nsfslite_error (n));
      ret = -1;
      goto cleanup;
    }

  if (nsfslite_remove (n, id, NULL, NULL, sizeof (int),
                       (struct nsfslite_stride){ .bstart = 100000 * sizeof (int), .stride = 1, .nelems = 50000 })
      < 0)
    {
      fprintf (stderr, "Failed to remove: %s\n", nsfslite_error (n));
      ret = -1;
      goto cleanup;
    }

  nsfslite_close (n);

  n = nsfslite_open ("test12.db", "test12.wal");
  if (!n)
    {
      fprintf (stderr, "Failed to reopen database\n");
      ret = -1;
      goto cleanup;
    }

  id = nsfslite_get_id (n, "data");
  if (id < 0)
    {
      fprintf (stderr, "Failed to get id: %s\n", nsfslite_error (n));
      ret = -1;
      goto cleanup;
    }

  size_t remaining = N_ELEMS - 50000;
  if (nsfslite_read (n, id, read_data, sizeof (int),
                     (struct nsfslite_stride){ .bstart = 0, .stride = 1, .nelems = remaining })
      < 0)
    {
      fprintf (stderr, "Failed to read: %s\n", nsfslite_error (n));
      ret = -1;
      goto cleanup;
    }

  for (size_t i = 50000; i < 60000; i++)
    {
      int expected = 55000 + (i - 50000);
      if (read_data[i] != expected)
        {
          fprintf (stderr, "Mismatch in updated region at %zu: expected %d, got %d\n", i, expected, read_data[i]);
          ret = -1;
          goto cleanup;
        }
    }

  printf ("SUCCESS: Mixed insert/write/remove operations verified after reopen\n");

cleanup:
  if (n)
    nsfslite_close (n);
  free (data);
  free (read_data);
  free (update);
  return ret;
}
