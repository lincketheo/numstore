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
 *   Integration tests for adaptive hash table (adptv_hash_table)
 */

#include <numstore/test/testing.h>

#include <config.h>
#include <numstore/core/adptv_hash_table.h>
#include <numstore/core/error.h>
#include <numstore/core/macros.h>

#include <stdlib.h>

#ifndef NTEST

// Test data structure
struct test_entry
{
  u32 key;
  u32 value;
  struct hnode node;
};

static void
test_entry_init (struct test_entry *e, u32 key, u32 value)
{
  e->key = key;
  e->value = value;
  hnode_init (&e->node, key);
}

static bool
test_entry_equals (const struct hnode *left, const struct hnode *right)
{
  const struct test_entry *l = container_of (left, const struct test_entry, node);
  const struct test_entry *r = container_of (right, const struct test_entry, node);
  return l->key == r->key;
}

// Context for foreach tests
struct foreach_ctx
{
  u32 count;
  u32 sum;
};

static void
count_and_sum (struct hnode *node, void *ctx)
{
  struct foreach_ctx *fctx = ctx;
  struct test_entry *entry = container_of (node, struct test_entry, node);
  fctx->count++;
  fctx->sum += entry->value;
}

// Basic operations test
TEST (TT_UNIT, adptv_htable_basic_operations)
{
  error e = error_create ();
  struct adptv_htable table;
  struct adptv_htable_settings settings = {
    .max_load_factor = 8,
    .min_load_factor = 1,
    .rehashing_work = 10,
    .max_size = 1024,
    .min_size = 4
  };

  test_err_t_wrap (adptv_htable_init (&table, settings, &e), &e);

  TEST_CASE ("Insert and lookup")
  {
    struct test_entry *entries = malloc (sizeof (struct test_entry) * 10);
    for (u32 i = 0; i < 10; i++)
      {
        test_entry_init (&entries[i], i, i * 100);
        test_err_t_wrap (adptv_htable_insert (&table, &entries[i].node, &e), &e);
      }

    test_assert_int_equal (adptv_htable_size (&table), 10);

    // Lookup all entries
    for (u32 i = 0; i < 10; i++)
      {
        struct test_entry key;
        test_entry_init (&key, i, 0);
        struct hnode *found = adptv_htable_lookup (&table, &key.node, test_entry_equals);
        test_fail_if_null (found);
        struct test_entry *entry = container_of (found, struct test_entry, node);
        test_assert_int_equal (entry->key, i);
        test_assert_int_equal (entry->value, i * 100);
      }
  }

  TEST_CASE ("Lookup non-existent")
  {
    struct test_entry key;
    test_entry_init (&key, 999, 0);
    struct hnode *found = adptv_htable_lookup (&table, &key.node, test_entry_equals);
    test_assert (found == NULL);
  }

  TEST_CASE ("Delete and verify")
  {
    struct test_entry key;
    test_entry_init (&key, 5, 0);
    struct hnode *deleted = NULL;
    test_err_t_wrap (adptv_htable_delete (&deleted, &table, &key.node, test_entry_equals, &e), &e);
    test_fail_if_null (deleted);

    struct test_entry *entry = container_of (deleted, struct test_entry, node);
    test_assert_int_equal (entry->key, 5);
    test_assert_int_equal (entry->value, 500);

    test_assert_int_equal (adptv_htable_size (&table), 9);

    // Verify it's really deleted
    struct hnode *found = adptv_htable_lookup (&table, &key.node, test_entry_equals);
    test_assert (found == NULL);
  }

  TEST_CASE ("Delete non-existent")
  {
    struct test_entry key;
    test_entry_init (&key, 999, 0);
    struct hnode *deleted = NULL;
    test_err_t_wrap (adptv_htable_delete (&deleted, &table, &key.node, test_entry_equals, &e), &e);
    test_assert (deleted == NULL);
  }

  adptv_htable_free (&table);
}

// Test resizing up (growth)
TEST (TT_UNIT, adptv_htable_resize_up)
{
  error e = error_create ();
  struct adptv_htable table;
  struct adptv_htable_settings settings = {
    .max_load_factor = 2, // Low threshold to trigger resize quickly
    .min_load_factor = 1,
    .rehashing_work = 5, // Small work units to test incremental rehashing
    .max_size = 256,
    .min_size = 4
  };

  test_err_t_wrap (adptv_htable_init (&table, settings, &e), &e);

  TEST_CASE ("Insert elements to trigger resize")
  {
    // Initial capacity is min_size = 4
    // With max_load_factor = 2, threshold = 4 * 2 = 8
    // Inserting 8+ elements should trigger resize

    struct test_entry *entries = malloc (sizeof (struct test_entry) * 100);

    for (u32 i = 0; i < 50; i++)
      {
        test_entry_init (&entries[i], i, i * 10);
        test_err_t_wrap (adptv_htable_insert (&table, &entries[i].node, &e), &e);
      }

    test_assert_int_equal (adptv_htable_size (&table), 50);

    // Verify all elements are still accessible
    for (u32 i = 0; i < 50; i++)
      {
        struct test_entry key;
        test_entry_init (&key, i, 0);
        struct hnode *found = adptv_htable_lookup (&table, &key.node, test_entry_equals);
        test_fail_if_null (found);
        struct test_entry *entry = container_of (found, struct test_entry, node);
        test_assert_int_equal (entry->value, i * 10);
      }

    // Table should have grown significantly
    test_assert (table.current->cap > 4);
    // All rehashing should be complete (prev should be empty)
    test_assert_int_equal (table.prev->size, 0);
  }

  adptv_htable_free (&table);
}

// Test resizing down (shrinkage)
TEST (TT_UNIT, adptv_htable_resize_down)
{
  error e = error_create ();
  struct adptv_htable table;
  struct adptv_htable_settings settings = {
    .max_load_factor = 4,
    .min_load_factor = 1, // Shrink when size <= cap * 1
    .rehashing_work = 5,
    .max_size = 256,
    .min_size = 4
  };

  test_err_t_wrap (adptv_htable_init (&table, settings, &e), &e);

  TEST_CASE ("Grow then shrink")
  {
    struct test_entry *entries = malloc (sizeof (struct test_entry) * 100);

    // First, insert many elements to grow the table
    for (u32 i = 0; i < 80; i++)
      {
        test_entry_init (&entries[i], i, i * 10);
        test_err_t_wrap (adptv_htable_insert (&table, &entries[i].node, &e), &e);
      }

    u32 cap_after_growth = table.current->cap;
    test_assert (cap_after_growth > 4);
    test_assert_int_equal (adptv_htable_size (&table), 80);

    // Now delete most elements to trigger shrinkage
    for (u32 i = 0; i < 75; i++)
      {
        struct test_entry key;
        test_entry_init (&key, i, 0);
        struct hnode *deleted = NULL;
        test_err_t_wrap (adptv_htable_delete (&deleted, &table, &key.node, test_entry_equals, &e), &e);
        test_fail_if_null (deleted);
      }

    test_assert_int_equal (adptv_htable_size (&table), 5);

    // Table should have shrunk
    test_assert (table.current->cap < cap_after_growth);

    // Verify remaining elements are still accessible
    for (u32 i = 75; i < 80; i++)
      {
        struct test_entry key;
        test_entry_init (&key, i, 0);
        struct hnode *found = adptv_htable_lookup (&table, &key.node, test_entry_equals);
        test_fail_if_null (found);
        struct test_entry *entry = container_of (found, struct test_entry, node);
        test_assert_int_equal (entry->value, i * 10);
      }
  }

  adptv_htable_free (&table);
}

// Test operations during rehashing
TEST (TT_UNIT, adptv_htable_lookup_during_rehashing)
{
  error e = error_create ();
  struct adptv_htable table;
  struct adptv_htable_settings settings = {
    .max_load_factor = 2,
    .min_load_factor = 1,
    .rehashing_work = 2, // Very small work units to keep rehashing active
    .max_size = 256,
    .min_size = 4
  };

  test_err_t_wrap (adptv_htable_init (&table, settings, &e), &e);

  TEST_CASE ("Lookup elements in both tables during rehashing")
  {
    struct test_entry *entries = malloc (sizeof (struct test_entry) * 50);

    // Insert elements to trigger resize
    for (u32 i = 0; i < 20; i++)
      {
        test_entry_init (&entries[i], i, i * 100);
        test_err_t_wrap (adptv_htable_insert (&table, &entries[i].node, &e), &e);
      }

    // At this point, some elements might still be in prev table
    // due to incremental rehashing

    // All elements should be findable regardless of which table they're in
    for (u32 i = 0; i < 20; i++)
      {
        struct test_entry key;
        test_entry_init (&key, i, 0);
        struct hnode *found = adptv_htable_lookup (&table, &key.node, test_entry_equals);
        test_fail_if_null (found);
        struct test_entry *entry = container_of (found, struct test_entry, node);
        test_assert_int_equal (entry->value, i * 100);
      }
  }

  adptv_htable_free (&table);
}

// Test foreach iteration
TEST (TT_UNIT, adptv_htable_foreach)
{
  error e = error_create ();
  struct adptv_htable table;
  struct adptv_htable_settings settings = {
    .max_load_factor = 8,
    .min_load_factor = 1,
    .rehashing_work = 10,
    .max_size = 1024,
    .min_size = 4
  };

  test_err_t_wrap (adptv_htable_init (&table, settings, &e), &e);

  TEST_CASE ("Iterate over all elements")
  {
    struct test_entry *entries = malloc (sizeof (struct test_entry) * 20);
    u32 expected_sum = 0;

    for (u32 i = 0; i < 20; i++)
      {
        test_entry_init (&entries[i], i, i);
        expected_sum += i;
        test_err_t_wrap (adptv_htable_insert (&table, &entries[i].node, &e), &e);
      }

    struct foreach_ctx ctx = { .count = 0, .sum = 0 };
    adptv_htable_foreach (&table, count_and_sum, &ctx);

    test_assert_int_equal (ctx.count, 20);
    test_assert_int_equal (ctx.sum, expected_sum);
  }

  adptv_htable_free (&table);
}

// Test min/max size constraints
TEST (TT_UNIT, adptv_htable_size_constraints)
{
  error e = error_create ();
  struct adptv_htable table;
  struct adptv_htable_settings settings = {
    .max_load_factor = 2,
    .min_load_factor = 1,
    .rehashing_work = 10,
    .max_size = 16, // Small max size
    .min_size = 4
  };

  test_err_t_wrap (adptv_htable_init (&table, settings, &e), &e);

  TEST_CASE ("Respect max_size constraint")
  {
    struct test_entry *entries = malloc (sizeof (struct test_entry) * 100);

    // Insert many elements
    for (u32 i = 0; i < 100; i++)
      {
        test_entry_init (&entries[i], i, i);
        test_err_t_wrap (adptv_htable_insert (&table, &entries[i].node, &e), &e);
      }

    // Table capacity should not exceed max_size
    test_assert (table.current->cap <= settings.max_size);

    // All elements should still be accessible
    for (u32 i = 0; i < 100; i++)
      {
        struct test_entry key;
        test_entry_init (&key, i, 0);
        struct hnode *found = adptv_htable_lookup (&table, &key.node, test_entry_equals);
        test_fail_if_null (found);
      }
  }

  adptv_htable_free (&table);

  // Test min_size constraint
  struct adptv_htable table2;
  test_err_t_wrap (adptv_htable_init (&table2, settings, &e), &e);

  TEST_CASE ("Respect min_size constraint")
  {
    struct test_entry *entries = malloc (sizeof (struct test_entry) * 10);

    // Insert a few elements
    for (u32 i = 0; i < 10; i++)
      {
        test_entry_init (&entries[i], i, i);
        test_err_t_wrap (adptv_htable_insert (&table2, &entries[i].node, &e), &e);
      }

    // Delete most of them
    for (u32 i = 0; i < 9; i++)
      {
        struct test_entry key;
        test_entry_init (&key, i, 0);
        struct hnode *deleted = NULL;
        test_err_t_wrap (adptv_htable_delete (&deleted, &table2, &key.node, test_entry_equals, &e), &e);
      }

    // Table capacity should not go below min_size
    test_assert (table2.current->cap >= settings.min_size);

    // Remaining element should still be accessible
    struct test_entry key;
    test_entry_init (&key, 9, 0);
    struct hnode *found = adptv_htable_lookup (&table2, &key.node, test_entry_equals);
    test_fail_if_null (found);
  }

  adptv_htable_free (&table2);
}

// Stress test: multiple resize cycles
TEST (TT_UNIT, adptv_htable_resize_stress)
{
  error e = error_create ();
  struct adptv_htable table;
  struct adptv_htable_settings settings = {
    .max_load_factor = 3,
    .min_load_factor = 1,
    .rehashing_work = 8,
    .max_size = 512,
    .min_size = 4
  };

  test_err_t_wrap (adptv_htable_init (&table, settings, &e), &e);

  TEST_CASE ("Multiple grow and shrink cycles")
  {
    struct test_entry *entries = malloc (sizeof (struct test_entry) * 200);

    for (u32 i = 0; i < 200; i++)
      {
        test_entry_init (&entries[i], i, i);
      }

    // Cycle 1: Grow
    for (u32 i = 0; i < 100; i++)
      {
        test_err_t_wrap (adptv_htable_insert (&table, &entries[i].node, &e), &e);
      }
    u32 size1 = adptv_htable_size (&table);
    test_assert_int_equal (size1, 100);

    // Cycle 1: Shrink
    for (u32 i = 0; i < 90; i++)
      {
        struct test_entry key;
        test_entry_init (&key, i, 0);
        struct hnode *deleted = NULL;
        test_err_t_wrap (adptv_htable_delete (&deleted, &table, &key.node, test_entry_equals, &e), &e);
      }
    u32 size2 = adptv_htable_size (&table);
    test_assert_int_equal (size2, 10);

    // Cycle 2: Grow again
    for (u32 i = 100; i < 200; i++)
      {
        test_err_t_wrap (adptv_htable_insert (&table, &entries[i].node, &e), &e);
      }
    u32 size3 = adptv_htable_size (&table);
    test_assert_int_equal (size3, 110);

    // Verify all remaining elements
    for (u32 i = 90; i < 100; i++)
      {
        struct test_entry key;
        test_entry_init (&key, i, 0);
        struct hnode *found = adptv_htable_lookup (&table, &key.node, test_entry_equals);
        test_fail_if_null (found);
      }
    for (u32 i = 100; i < 200; i++)
      {
        struct test_entry key;
        test_entry_init (&key, i, 0);
        struct hnode *found = adptv_htable_lookup (&table, &key.node, test_entry_equals);
        test_fail_if_null (found);
      }
  }

  adptv_htable_free (&table);
}

#endif
