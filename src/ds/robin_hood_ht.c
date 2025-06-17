#include "ds/robin_hood_ht.h"

#include "dev/assert.h"
#include "dev/testing.h"
#include "errors/error.h"
#include "intf/io.h"
#include "intf/logging.h"
#include <strings.h>

DEFINE_DBG_ASSERT_I (hash_table, hash_table, h)
{
  ASSERT (h);
  ASSERT (h->cap > 0);
}

static const char *TAG = "Hash Table";

hash_table *
ht_open (u32 nelem, error *e)
{
  hash_table *ret = i_malloc (1, sizeof *ret + nelem * sizeof *ret->elems);

  if (ret == NULL)
    {
      error_causef (e, ERR_NOMEM, "%s Failed to allocate hash table", TAG);
      return NULL;
    }

  ret->cap = nelem;

  return ret;
}
void
ht_close (hash_table *ht)
{
  hash_table_assert (ht);
  i_free (ht);
}

hti_res
ht_insert (hash_table *ht, hdata data)
{
  hash_table_assert (ht);

  u32 dibn = 0; // Current distance from initial bucket

  for (u32 i = 0; i < ht->cap; ++i, ++dibn)
    {
      // Mapped index after probing
      u32 _i = (data.key + i) % ht->cap;

      // If not present, insert
      if (!ht->elems[_i].present)
        {
          ht->elems[_i].data = data;
          ht->elems[_i].dib = dibn;
          ht->elems[_i].present = true;
          return HTIR_SUCCESS;
        }

      // Swap (lt means dib != dibn, therefore key != key)
      if (ht->elems[_i].dib < dibn)
        {
          hdata temp_data = ht->elems[_i].data;
          u32 temp_dib = ht->elems[_i].dib;

          ht->elems[_i].data = data;
          ht->elems[_i].dib = dibn;

          dibn = temp_dib;
          data = temp_data;
        }

      // Compare keys for duplicates
      if (ht->elems[_i].data.key == data.key)
        {
          return HTIR_EXISTS;
        }
    }

  return HTIR_FULL;
}

hta_res
ht_get (const hash_table *ht, hdata *dest, hkey key)
{
  hash_table_assert (ht);

  u32 dibn = 0;

  for (u32 i = 0; i < ht->cap; ++i, ++dibn)
    {
      // Mapped index after probing
      u32 _i = (key + i) % ht->cap;

      // If not present, return
      if (!ht->elems[_i].present)
        {
          return HTAR_DOESNT_EXIST;
        }

      // Short cut - DIB invariant is broken
      if (ht->elems[_i].dib < dibn)
        {
          return HTAR_DOESNT_EXIST;
        }

      // Check for key
      if (ht->elems[_i].data.key == key)
        {
          if (dest)
            {
              *dest = ht->elems[_i].data;
            }
          return HTAR_SUCCESS;
        }
    }

  return HTAR_DOESNT_EXIST;
}

hta_res
ht_delete (hash_table *ht, hkey key)
{
  hash_table_assert (ht);

  u32 dibn = 0;
  u32 i = 0;

  for (i = 0; i < ht->cap; ++i, ++dibn)
    {
      // Mapped index after probing
      u32 _i = (key + i) % ht->cap;

      // If not present, return
      if (!ht->elems[_i].present)
        {
          return HTAR_DOESNT_EXIST;
        }

      // Short cut - DIB invariant is broken
      if (ht->elems[_i].dib < dibn)
        {
          return HTAR_DOESNT_EXIST;
        }

      // Check for key
      if (ht->elems[_i].data.key == key)
        {
          break;
        }
    }

  // Shift all full elements to the left
  for (; i < ht->cap; ++i)
    {
      // Mapped index after probing
      u32 hole = (key + i) % ht->cap;
      u32 next = (key + i + 1) % ht->cap;

      // Right value is empty, set this to empty and return
      if (!ht->elems[next].present || ht->elems[next].dib == 0)
        {
          ht->elems[hole].present = false;
          return HTAR_SUCCESS;
        }

      // Shift left
      ht->elems[hole].data = ht->elems[next].data;
      ASSERT (ht->elems[next].dib > 0);
      ht->elems[hole].dib = ht->elems[next].dib - 1;
      ASSERT (ht->elems[hole].present);
    }

  return HTAR_DOESNT_EXIST;
}

TEST (robin_hood_ht)
{
  error e = error_create (NULL);
  hash_table *ht = ht_open (100, &e);
  test_fail_if_null (ht);

  test_assert_int_equal (
      ht_insert (ht, (hdata){ .key = 10u, .index = 5 }),
      HTIR_SUCCESS);

  // Duplicate insert
  test_assert_int_equal (
      ht_insert (ht, (hdata){ .key = 10u, .index = 99 }),
      HTIR_EXISTS);

  // Get normal
  hdata out;
  test_assert_int_equal (
      ht_get (ht, &out, 10u),
      HTAR_SUCCESS);
  test_assert_int_equal (out.index, 5);

  // Miss look up
  test_assert_int_equal (
      ht_get (ht, &out, 123u),
      HTAR_DOESNT_EXIST);

  // Delete normal
  test_assert_int_equal (
      ht_delete (ht, 10u),
      HTAR_SUCCESS);
  test_assert_int_equal (
      ht_get (ht, &out, 10u),
      HTAR_DOESNT_EXIST);

  // Delete doesn't exist
  test_assert_int_equal (
      ht_delete (ht, 10u),
      HTAR_DOESNT_EXIST);

  // Linear probing
  test_assert_int_equal (
      ht_insert (ht, (hdata){ .key = 10u, .index = 10 }),
      HTIR_SUCCESS);
  test_assert_int_equal (
      ht_insert (ht, (hdata){ .key = 110u, .index = 11 }),
      HTIR_SUCCESS);
  test_assert_int_equal (
      ht_insert (ht, (hdata){ .key = 210u, .index = 21 }),
      HTIR_SUCCESS);

  // Delete and create a hole
  test_assert_int_equal (
      ht_delete (ht, 10u),
      HTAR_SUCCESS);

  // Fetch the rest
  test_assert_int_equal (
      ht_get (ht, &out, 110u),
      HTAR_SUCCESS);
  test_assert_int_equal (out.index, 11);
  test_assert_int_equal (
      ht_get (ht, &out, 210u),
      HTAR_SUCCESS);
  test_assert_int_equal (out.index, 21);

  // Optional dest
  test_assert_int_equal (
      ht_get (ht, NULL, 110u),
      HTAR_SUCCESS);

  // Full table
  hash_table *tiny = ht_open (4, &e); // TODO - this test is failing only on github
  test_fail_if_null (tiny);
  for (u32 k = 0; k < 4; ++k)
    {
      test_assert_int_equal (
          ht_insert (tiny, (hdata){ .key = k, .index = k }),
          HTIR_SUCCESS);
    }

  test_assert_int_equal (
      ht_insert (tiny, (hdata){ .key = 99u, .index = 99 }),
      HTIR_FULL);

  ht_close (tiny);
  ht_close (ht);
}

void
i_log_ht (const hash_table *ht)
{
  hash_table_assert (ht);
  i_log_info ("========= HASH TABLE START ========\n");
  for (u32 i = 0; i < ht->cap; ++i)
    {
      if (ht->elems[i].present)
        {
          i_log_info ("[%d] %u %" PRpgno "\n", i,
                      ht->elems[i].data.index,
                      ht->elems[i].data.key);
        }
    }
  i_log_info ("========= HASH TABLE END ========\n");
}
