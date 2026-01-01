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
 * Example 8: Strided Remove
 *
 * Demonstrates removing data with a stride pattern. Shows how to delete elements at
 * regular intervals, useful for filtering or downsampling datasets.
 *
 * Usage:
 *   ./example8_remove_stride
 */

#include "nsfslite.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define N_ELEMS 20000
#define REMOVE_STRIDE 10
#define REMOVE_COUNT 1000

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

  unlink ("test8.db");
  unlink ("test8.wal");

  n = nsfslite_open ("test8.db", "test8.wal");
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

  if (nsfslite_remove (n, id, NULL, NULL, sizeof (int),
                       (struct nsfslite_stride){ .bstart = 0, .stride = REMOVE_STRIDE, .nelems = REMOVE_COUNT })
      < 0)
    {
      fprintf (stderr, "Failed to remove with stride: %s\n", nsfslite_error (n));
      ret = -1;
      goto cleanup;
    }

  nsfslite_close (n);

  n = nsfslite_open ("test8.db", "test8.wal");
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

  size_t remaining = N_ELEMS - REMOVE_COUNT;
  if (nsfslite_read (n, id, read_data, sizeof (int),
                     (struct nsfslite_stride){ .bstart = 0, .stride = 1, .nelems = remaining })
      < 0)
    {
      fprintf (stderr, "Failed to read: %s\n", nsfslite_error (n));
      ret = -1;
      goto cleanup;
    }

  printf ("SUCCESS: Removed %d elements with stride=%d, %zu remaining after reopen\n", REMOVE_COUNT, REMOVE_STRIDE, remaining);

cleanup:
  if (n)
    nsfslite_close (n);
  free (data);
  free (read_data);
  return ret;
}
