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
 * Example 5: Offset-based Operations
 *
 * Demonstrates offset-based write and read operations. Shows how to write and read
 * data starting at a specific byte offset in the storage, enabling random access
 * patterns.
 *
 * Usage:
 *   ./example5_offset
 */

#include "nsfslite.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define N_ELEMS 200000
#define OFFSET 1000

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

  unlink ("test5.db");
  unlink ("test5.wal");

  n = nsfslite_open ("test5.db", "test5.wal");
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

  if (nsfslite_write (n, id, NULL, data, sizeof (int),
                      (struct nsfslite_stride){ .bstart = OFFSET * sizeof (int), .stride = 1, .nelems = N_ELEMS })
      < 0)
    {
      fprintf (stderr, "Failed to write with offset: %s\n", nsfslite_error (n));
      ret = -1;
      goto cleanup;
    }

  nsfslite_close (n);

  n = nsfslite_open ("test5.db", "test5.wal");
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

  if (nsfslite_read (n, id, read_data, sizeof (int),
                     (struct nsfslite_stride){ .bstart = OFFSET * sizeof (int), .stride = 1, .nelems = N_ELEMS })
      < 0)
    {
      fprintf (stderr, "Failed to read with offset: %s\n", nsfslite_error (n));
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

  printf ("SUCCESS: All %d elements match with offset=%d after reopen\n", N_ELEMS, OFFSET);

cleanup:
  if (n)
    nsfslite_close (n);
  free (data);
  free (read_data);
  return ret;
}
