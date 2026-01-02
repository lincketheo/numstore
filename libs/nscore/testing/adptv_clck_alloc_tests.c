/*
 * Copyright 2025 Theo Lincke
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
 * Description:
 *   Tests for adaptive clock allocator (adptv_clck_alloc)
 */

#include <numstore/test/testing.h>

#include <config.h>
#include <numstore/core/adptv_clck_alloc.h>
#include <numstore/core/error.h>

#include <stdlib.h>
#include <string.h>

#ifndef NTEST

// Basic operations test
TEST (TT_UNIT, adptv_clck_alloc_basic_operations)
{
  error e = error_create ();
  struct adptv_clck_alloc *aca;
  struct adptv_clck_alloc_settings settings = {
    .migration_work = 4,
    .max_capacity = 1024,
    .min_capacity = 4
  };

  test_err_t_wrap (adptv_clck_alloc_init (&aca, sizeof (i32), 8, settings, &e), &e);

  TEST_CASE ("Initial state")
  {
    test_assert_int_equal (adptv_clck_alloc_size (aca), 0);
    test_assert_int_equal (adptv_clck_alloc_capacity (aca), 8);
  }

  TEST_CASE ("Allocate and write")
  {
    i32 handle1 = adptv_clck_alloc_alloc (aca, &e);
    test_err_t_wrap (e.cause_code, &e);
    test_assert (handle1 >= 0);
    test_assert_int_equal (adptv_clck_alloc_size (aca), 1);

    i32 *ptr1 = (i32 *)adptv_clck_alloc_get_at (aca, handle1);
    test_fail_if_null (ptr1);
    *ptr1 = 42;

    i32 handle2 = adptv_clck_alloc_alloc (aca, &e);
    test_err_t_wrap (e.cause_code, &e);
    test_assert (handle2 >= 0);
    test_assert (handle2 != handle1);
    test_assert_int_equal (adptv_clck_alloc_size (aca), 2);

    i32 *ptr2 = (i32 *)adptv_clck_alloc_get_at (aca, handle2);
    test_fail_if_null (ptr2);
    *ptr2 = 100;

    // Verify values
    test_assert_int_equal (*((i32 *)adptv_clck_alloc_get_at (aca, handle1)), 42);
    test_assert_int_equal (*((i32 *)adptv_clck_alloc_get_at (aca, handle2)), 100);
  }

  TEST_CASE ("Calloc zeroes memory")
  {
    i32 handle = adptv_clck_alloc_calloc (aca, &e);
    test_err_t_wrap (e.cause_code, &e);
    test_assert (handle >= 0);

    i32 *ptr = (i32 *)adptv_clck_alloc_get_at (aca, handle);
    test_fail_if_null (ptr);
    test_assert_int_equal (*ptr, 0);
  }

  TEST_CASE ("Free and reallocate")
  {
    i32 handle1 = adptv_clck_alloc_alloc (aca, &e);
    test_err_t_wrap (e.cause_code, &e);
    i32 *ptr1 = (i32 *)adptv_clck_alloc_get_at (aca, handle1);
    *ptr1 = 999;

    u32 size_before = adptv_clck_alloc_size (aca);
    adptv_clck_alloc_free (aca, handle1, &e);
    test_err_t_wrap (e.cause_code, &e);
    test_assert_int_equal (adptv_clck_alloc_size (aca), size_before - 1);

    // get_at on freed handle should return NULL
    test_assert (adptv_clck_alloc_get_at (aca, handle1) == NULL);

    // Reallocate
    i32 handle2 = adptv_clck_alloc_alloc (aca, &e);
    test_err_t_wrap (e.cause_code, &e);
    test_assert (handle2 >= 0);
    test_assert_int_equal (adptv_clck_alloc_size (aca), size_before);
  }

  adptv_clck_alloc_free_allocator (aca);
}

// Test expansion (doubling capacity)
TEST (TT_UNIT, adptv_clck_alloc_resize_up)
{
  error e = error_create ();
  struct adptv_clck_alloc *aca;
  struct adptv_clck_alloc_settings settings = {
    .migration_work = 2,  // Small work units to test incremental migration
    .max_capacity = 256,
    .min_capacity = 4
  };

  test_err_t_wrap (adptv_clck_alloc_init (&aca, sizeof (u32), 4, settings, &e), &e);

  TEST_CASE ("Fill to capacity and trigger expansion")
  {
    test_assert_int_equal (adptv_clck_alloc_capacity (aca), 4);

    i32 handles[20];

    // Allocate 4 elements (fill initial capacity)
    for (u32 i = 0; i < 4; ++i)
      {
        handles[i] = adptv_clck_alloc_alloc (aca, &e);
        test_err_t_wrap (e.cause_code, &e);
        test_assert (handles[i] >= 0);

        u32 *ptr = (u32 *)adptv_clck_alloc_get_at (aca, handles[i]);
        *ptr = i * 10;
      }

    test_assert_int_equal (adptv_clck_alloc_size (aca), 4);
    test_assert_int_equal (adptv_clck_alloc_capacity (aca), 4);

    // Next allocation should trigger expansion
    handles[4] = adptv_clck_alloc_alloc (aca, &e);
    test_err_t_wrap (e.cause_code, &e);
    test_assert (handles[4] >= 0);
    test_assert_int_equal (adptv_clck_alloc_size (aca), 5);
    test_assert_int_equal (adptv_clck_alloc_capacity (aca), 8);

    u32 *ptr = (u32 *)adptv_clck_alloc_get_at (aca, handles[4]);
    *ptr = 40;

    // Continue allocating to trigger more migration
    for (u32 i = 5; i < 10; ++i)
      {
        handles[i] = adptv_clck_alloc_alloc (aca, &e);
        test_err_t_wrap (e.cause_code, &e);
        u32 *ptr = (u32 *)adptv_clck_alloc_get_at (aca, handles[i]);
        *ptr = i * 10;
      }

    // Verify all values are intact
    for (u32 i = 0; i < 10; ++i)
      {
        u32 *ptr = (u32 *)adptv_clck_alloc_get_at (aca, handles[i]);
        test_fail_if_null (ptr);
        test_assert_int_equal (*ptr, i * 10);
      }

    test_assert_int_equal (adptv_clck_alloc_size (aca), 10);
  }

  adptv_clck_alloc_free_allocator (aca);
}

// Test contraction (halving capacity)
TEST (TT_UNIT, adptv_clck_alloc_resize_down)
{
  error e = error_create ();
  struct adptv_clck_alloc *aca;
  struct adptv_clck_alloc_settings settings = {
    .migration_work = 4,
    .max_capacity = 256,
    .min_capacity = 4
  };

  test_err_t_wrap (adptv_clck_alloc_init (&aca, sizeof (u64), 16, settings, &e), &e);

  TEST_CASE ("Grow then shrink")
  {
    i32 handles[20];

    // Allocate 16 elements (fill capacity)
    for (u32 i = 0; i < 16; ++i)
      {
        handles[i] = adptv_clck_alloc_alloc (aca, &e);
        test_err_t_wrap (e.cause_code, &e);
        u64 *ptr = (u64 *)adptv_clck_alloc_get_at (aca, handles[i]);
        *ptr = i * 1000;
      }

    test_assert_int_equal (adptv_clck_alloc_size (aca), 16);
    test_assert_int_equal (adptv_clck_alloc_capacity (aca), 16);

    // Trigger expansion to 32
    handles[16] = adptv_clck_alloc_alloc (aca, &e);
    test_err_t_wrap (e.cause_code, &e);
    u64 *ptr = (u64 *)adptv_clck_alloc_get_at (aca, handles[16]);
    *ptr = 16000;

    test_assert_int_equal (adptv_clck_alloc_capacity (aca), 32);

    // Free most elements to trigger shrinkage
    // Size threshold for shrink: capacity * 0.25 = 32 * 0.25 = 8
    // So when size <= 8, should shrink to 16
    for (u32 i = 0; i < 9; ++i)
      {
        adptv_clck_alloc_free (aca, handles[i], &e);
        test_err_t_wrap (e.cause_code, &e);
      }

    test_assert_int_equal (adptv_clck_alloc_size (aca), 8);

    // Should have triggered shrink from 32 to 16
    test_assert_int_equal (adptv_clck_alloc_capacity (aca), 16);

    // Verify remaining values are intact
    for (u32 i = 9; i < 17; ++i)
      {
        u64 *ptr = (u64 *)adptv_clck_alloc_get_at (aca, handles[i]);
        test_fail_if_null (ptr);
        test_assert_int_equal (*ptr, i * 1000);
      }
  }

  adptv_clck_alloc_free_allocator (aca);
}

// Test incremental migration during operations
TEST (TT_UNIT, adptv_clck_alloc_incremental_migration)
{
  error e = error_create ();
  struct adptv_clck_alloc *aca;
  struct adptv_clck_alloc_settings settings = {
    .migration_work = 1,  // Very small work units
    .max_capacity = 256,
    .min_capacity = 4
  };

  test_err_t_wrap (adptv_clck_alloc_init (&aca, sizeof (i32), 4, settings, &e), &e);

  TEST_CASE ("Data remains accessible during migration")
  {
    i32 handles[10];

    // Fill initial capacity
    for (u32 i = 0; i < 4; ++i)
      {
        handles[i] = adptv_clck_alloc_alloc (aca, &e);
        test_err_t_wrap (e.cause_code, &e);
        i32 *ptr = (i32 *)adptv_clck_alloc_get_at (aca, handles[i]);
        *ptr = 100 + i;
      }

    // Trigger expansion and migration
    for (u32 i = 4; i < 10; ++i)
      {
        handles[i] = adptv_clck_alloc_alloc (aca, &e);
        test_err_t_wrap (e.cause_code, &e);
        i32 *ptr = (i32 *)adptv_clck_alloc_get_at (aca, handles[i]);
        *ptr = 100 + i;

        // After each allocation, verify all previous values
        for (u32 j = 0; j <= i; ++j)
          {
            i32 *check = (i32 *)adptv_clck_alloc_get_at (aca, handles[j]);
            test_fail_if_null (check);
            test_assert_int_equal (*check, 100 + j);
          }
      }
  }

  adptv_clck_alloc_free_allocator (aca);
}

// Test min/max capacity constraints
TEST (TT_UNIT, adptv_clck_alloc_capacity_constraints)
{
  error e = error_create ();
  struct adptv_clck_alloc *aca;
  struct adptv_clck_alloc_settings settings = {
    .migration_work = 4,
    .max_capacity = 16,  // Small max
    .min_capacity = 4
  };

  test_err_t_wrap (adptv_clck_alloc_init (&aca, sizeof (i32), 8, settings, &e), &e);

  TEST_CASE ("Respect max_capacity")
  {
    i32 handles[20];

    // Fill to capacity 8
    for (u32 i = 0; i < 8; ++i)
      {
        handles[i] = adptv_clck_alloc_alloc (aca, &e);
        test_err_t_wrap (e.cause_code, &e);
      }

    // Should expand to 16
    handles[8] = adptv_clck_alloc_alloc (aca, &e);
    test_err_t_wrap (e.cause_code, &e);
    test_assert_int_equal (adptv_clck_alloc_capacity (aca), 16);

    // Fill to 16
    for (u32 i = 9; i < 16; ++i)
      {
        handles[i] = adptv_clck_alloc_alloc (aca, &e);
        test_err_t_wrap (e.cause_code, &e);
      }

    // Try to expand beyond max_capacity - should fail
    handles[16] = adptv_clck_alloc_alloc (aca, &e);
    test_assert (handles[16] < 0);
    test_assert_int_equal (e.cause_code, ERR_NOMEM);
    e.cause_code = SUCCESS;

    // Capacity should still be at max
    test_assert_int_equal (adptv_clck_alloc_capacity (aca), 16);
  }

  adptv_clck_alloc_free_allocator (aca);

  // Test min_capacity
  struct adptv_clck_alloc *aca2;
  test_err_t_wrap (adptv_clck_alloc_init (&aca2, sizeof (i32), 16, settings, &e), &e);

  TEST_CASE ("Respect min_capacity")
  {
    i32 handles[5];

    // Allocate a few elements
    for (u32 i = 0; i < 5; ++i)
      {
        handles[i] = adptv_clck_alloc_alloc (aca2, &e);
        test_err_t_wrap (e.cause_code, &e);
      }

    test_assert_int_equal (adptv_clck_alloc_capacity (aca2), 16);

    // Free most to trigger shrinkage
    for (u32 i = 0; i < 4; ++i)
      {
        adptv_clck_alloc_free (aca2, handles[i], &e);
        test_err_t_wrap (e.cause_code, &e);
      }

    // Should shrink to 8, then potentially to 4 (min_capacity)
    // but should not go below min_capacity
    test_assert (adptv_clck_alloc_capacity (aca2) >= settings.min_capacity);
  }

  adptv_clck_alloc_free_allocator (aca2);
}

// Stress test: many allocations and frees
TEST (TT_UNIT, adptv_clck_alloc_stress)
{
  error e = error_create ();
  struct adptv_clck_alloc *aca;
  struct adptv_clck_alloc_settings settings = {
    .migration_work = 3,
    .max_capacity = 128,
    .min_capacity = 4
  };

  test_err_t_wrap (adptv_clck_alloc_init (&aca, sizeof (u32), 8, settings, &e), &e);

  TEST_CASE ("Multiple allocation and free cycles")
  {
    i32 handles[100];

    // Allocate many
    for (u32 i = 0; i < 100; ++i)
      {
        handles[i] = adptv_clck_alloc_alloc (aca, &e);
        test_err_t_wrap (e.cause_code, &e);
        u32 *ptr = (u32 *)adptv_clck_alloc_get_at (aca, handles[i]);
        *ptr = i;
      }

    test_assert_int_equal (adptv_clck_alloc_size (aca), 100);

    // Verify all
    for (u32 i = 0; i < 100; ++i)
      {
        u32 *ptr = (u32 *)adptv_clck_alloc_get_at (aca, handles[i]);
        test_fail_if_null (ptr);
        test_assert_int_equal (*ptr, i);
      }

    // Free half
    for (u32 i = 0; i < 50; ++i)
      {
        adptv_clck_alloc_free (aca, handles[i], &e);
        test_err_t_wrap (e.cause_code, &e);
      }

    test_assert_int_equal (adptv_clck_alloc_size (aca), 50);

    // Verify remaining half
    for (u32 i = 50; i < 100; ++i)
      {
        u32 *ptr = (u32 *)adptv_clck_alloc_get_at (aca, handles[i]);
        test_fail_if_null (ptr);
        test_assert_int_equal (*ptr, i);
      }

    // Free rest
    for (u32 i = 50; i < 100; ++i)
      {
        adptv_clck_alloc_free (aca, handles[i], &e);
        test_err_t_wrap (e.cause_code, &e);
      }

    test_assert_int_equal (adptv_clck_alloc_size (aca), 0);
  }

  adptv_clck_alloc_free_allocator (aca);
}

// Test with larger element sizes
TEST (TT_UNIT, adptv_clck_alloc_large_elements)
{
  error e = error_create ();
  struct adptv_clck_alloc *aca;

  struct large_elem {
    u64 a, b, c, d;
    char data[256];
  };

  struct adptv_clck_alloc_settings settings = ADPTV_CLCK_ALLOC_DEFAULT_SETTINGS;

  test_err_t_wrap (adptv_clck_alloc_init (&aca, sizeof (struct large_elem), 4, settings, &e), &e);

  TEST_CASE ("Allocate and verify large elements")
  {
    i32 handles[10];

    for (u32 i = 0; i < 10; ++i)
      {
        handles[i] = adptv_clck_alloc_alloc (aca, &e);
        test_err_t_wrap (e.cause_code, &e);

        struct large_elem *elem = (struct large_elem *)adptv_clck_alloc_get_at (aca, handles[i]);
        test_fail_if_null (elem);

        elem->a = i;
        elem->b = i * 2;
        elem->c = i * 3;
        elem->d = i * 4;
        snprintf (elem->data, sizeof (elem->data), "Element %u", i);
      }

    // Verify
    for (u32 i = 0; i < 10; ++i)
      {
        struct large_elem *elem = (struct large_elem *)adptv_clck_alloc_get_at (aca, handles[i]);
        test_fail_if_null (elem);

        test_assert_int_equal (elem->a, i);
        test_assert_int_equal (elem->b, i * 2);
        test_assert_int_equal (elem->c, i * 3);
        test_assert_int_equal (elem->d, i * 4);

        char expected[256];
        snprintf (expected, sizeof (expected), "Element %u", i);
        test_assert (strcmp (elem->data, expected) == 0);
      }
  }

  adptv_clck_alloc_free_allocator (aca);
}

#endif
