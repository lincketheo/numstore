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
 * Example 3: Insert in Middle
 *
 * Demonstrates inserting data in the middle of existing data. Shows how nsfslite
 * can split existing data and insert new elements at a specified offset, shifting
 * subsequent elements.
 *
 * Usage:
 *   ./example3_insert_middle
 */

#include "nsfslite.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define N_ELEMS 20000
#define INSERT_COUNT 5000
#define INSERT_OFFSET 10000

#define CHECK(expr)                                                         \
  do                                                                        \
    {                                                                       \
      if ((expr) < 0)                                                       \
        {                                                                   \
          fprintf (stderr, "Failed: %s - %s\n", #expr, nsfslite_error (n)); \
          ret = -1;                                                         \
          goto cleanup;                                                     \
        }                                                                   \
    }                                                                       \
  while (0)

int
main (void)
{
  int ret = 0;
  nsfslite *n = NULL;
  int *data = malloc (N_ELEMS * sizeof (int));
  int *insert_data = malloc (INSERT_COUNT * sizeof (int));
  int *read_data = malloc ((N_ELEMS + INSERT_COUNT) * sizeof (int));
  int *expected = malloc ((N_ELEMS + INSERT_COUNT) * sizeof (int));

  for (size_t i = 0; i < N_ELEMS; i++)
    {
      data[i] = i;
    }
  for (size_t i = 0; i < INSERT_COUNT; i++)
    {
      insert_data[i] = 99000 + i;
    }

  for (size_t i = 0; i < INSERT_OFFSET; i++)
    {
      expected[i] = i;
    }
  for (size_t i = 0; i < INSERT_COUNT; i++)
    {
      expected[INSERT_OFFSET + i] = 99000 + i;
    }
  for (size_t i = INSERT_OFFSET; i < N_ELEMS; i++)
    {
      expected[INSERT_COUNT + i] = i;
    }

  unlink ("test3.db");
  unlink ("test3.wal");

  n = nsfslite_open ("test3.db", "test3.wal");
  if (!n)
    {
      fprintf (stderr, "Failed to open database\n");
      ret = -1;
      goto cleanup;
    }

  int64_t id = nsfslite_new (n, NULL, "data");
  CHECK (id);
  CHECK (nsfslite_insert (n, id, NULL, data, 0, sizeof (int), N_ELEMS));
  CHECK (nsfslite_insert (n, id, NULL, insert_data, INSERT_OFFSET * sizeof (int), sizeof (int), INSERT_COUNT));

  nsfslite_close (n);

  n = nsfslite_open ("test3.db", "test3.wal");
  if (!n)
    {
      fprintf (stderr, "Failed to reopen database\n");
      ret = -1;
      goto cleanup;
    }

  id = nsfslite_get_id (n, "data");
  CHECK (id);

  struct nsfslite_stride rstride = { .bstart = 0, .stride = 1, .nelems = N_ELEMS + INSERT_COUNT };
  CHECK (nsfslite_read (n, id, read_data, sizeof (int), rstride));

  for (size_t i = 0; i < N_ELEMS + INSERT_COUNT; i++)
    {
      if (expected[i] != read_data[i])
        {
          fprintf (stderr, "Mismatch at %zu: expected %d, got %d\n", i, expected[i], read_data[i]);
          ret = -1;
          goto cleanup;
        }
    }

  printf ("SUCCESS: Inserted %d elements in middle, verified after reopen\n", INSERT_COUNT);

cleanup:
  if (n)
    nsfslite_close (n);
  free (data);
  free (insert_data);
  free (read_data);
  free (expected);
  return ret;
}
