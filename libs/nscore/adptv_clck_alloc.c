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
 *   Adaptive clock allocator with dynamic capacity adjustment
 */

#include <numstore/core/adptv_clck_alloc.h>

#include <numstore/core/assert.h>
#include <numstore/core/error.h>
#include <numstore/intf/stdlib.h>
#include <string.h>

static void *
allocate_clck_memory (
    u32 capacity,
    size_t elem_size,
    bool **occupied_out,
    error *e)
{
  size_t data_size = capacity * elem_size;
  size_t occupied_size = capacity * sizeof (bool);
  size_t total_size = data_size + occupied_size;

  void *data = i_malloc (1, total_size, e);
  if (data == NULL)
    {
      return NULL;
    }

  i_memset (data, 0, total_size);
  *occupied_out = (bool *)((char *)data + data_size);

  return data;
}

static inline void *
get_element_ptr (void *data, u32 index, size_t elem_size)
{
  return (char *)data + (index * elem_size);
}

static i32
find_free_slot (void *data, bool *occupied, u32 *clock, u32 capacity)
{
  for (u32 i = 0; i < capacity; ++i)
    {
      u32 idx = *clock;
      *clock = (*clock + 1) % capacity;

      if (!occupied[idx])
        {
          return (i32)idx;
        }
    }
  return -1;
}

static void
finish_migration (struct adptv_clck_alloc *aca)
{
  if (aca->prev_data == NULL)
    {
      return; // No migration in progress
    }

  // Migrate all elements that are still in prev
  for (u32 h = 0; h < aca->handle_capacity; ++h)
    {
      if (aca->handle_table[h].allocated && !aca->handle_table[h].in_current)
        {
          u32 old_idx = aca->handle_table[h].phys_idx;

          // Find free slot in current
          i32 new_idx = find_free_slot (aca->current_data,
                                        aca->current_occupied,
                                        &aca->current_clock,
                                        aca->current_capacity);
          ASSERT (new_idx >= 0); // Must have space

          // Copy data
          void *src = get_element_ptr (aca->prev_data, old_idx, aca->elem_size);
          void *dst = get_element_ptr (aca->current_data, new_idx, aca->elem_size);
          memcpy (dst, src, aca->elem_size);

          // Mark slot as occupied BEFORE updating handle
          aca->current_occupied[new_idx] = true;

          // Update handle mapping
          aca->handle_table[h].in_current = true;
          aca->handle_table[h].phys_idx = new_idx;
        }
    }

  // Free previous allocator
  i_free (aca->prev_data);
  aca->prev_data = NULL;
  aca->prev_occupied = NULL;
  aca->prev_capacity = 0;
  aca->migrate_pos = 0;
}

static int
trigger_resize (
    struct adptv_clck_alloc *aca,
    u32 new_capacity,
    error *e)
{
  finish_migration (aca);

  bool *new_occupied;
  void *new_data = allocate_clck_memory (new_capacity, aca->elem_size, &new_occupied, e);

  if (new_data == NULL)
    {
      return error_change_causef (e, ERR_NOMEM, "Failed to allocate new capacity");
    }

  // Move current to prev
  aca->prev_data = aca->current_data;
  aca->prev_occupied = aca->current_occupied;
  aca->prev_capacity = aca->current_capacity;

  // Set new current
  aca->current_data = new_data;
  aca->current_occupied = new_occupied;
  aca->current_capacity = new_capacity;
  aca->current_clock = 0;
  aca->migrate_pos = 0;

  // Mark all allocated handles as being in prev now (not current)
  for (u32 h = 0; h < aca->handle_capacity; ++h)
    {
      if (aca->handle_table[h].allocated)
        {
          aca->handle_table[h].in_current = false;
        }
    }

  // Resize handle table if expanding
  if (new_capacity > aca->handle_capacity)
    {
      struct clock_frame *new_handles = i_malloc (new_capacity, sizeof (struct clock_frame), e);
      if (new_handles == NULL)
        {
          return error_change_causef (e, ERR_NOMEM, "Failed to resize handle table");
        }

      // Copy old entries
      for (u32 i = 0; i < aca->handle_capacity; ++i)
        {
          new_handles[i] = aca->handle_table[i];
        }

      // Zero out new entries
      for (u32 i = aca->handle_capacity; i < new_capacity; ++i)
        {
          new_handles[i].allocated = false;
          new_handles[i].in_current = false;
          new_handles[i].phys_idx = 0;
        }

      i_free (aca->handle_table);
      aca->handle_table = new_handles;
      aca->handle_capacity = new_capacity;
    }

  return SUCCESS;
}

int
adptv_clck_alloc_init (
    struct adptv_clck_alloc *aca,
    size_t elem_size,
    u32 initial_capacity,
    struct adptv_clck_alloc_settings settings,
    error *e)
{
  ASSERT (aca);
  ASSERT (elem_size > 0);
  ASSERT (initial_capacity > 0);
  ASSERT (settings.min_capacity == 0 || initial_capacity >= settings.min_capacity);
  ASSERT (settings.max_capacity == 0 || initial_capacity <= settings.max_capacity);

  aca->settings = settings;
  aca->elem_size = elem_size;
  aca->size = 0;
  aca->migrate_pos = 0;

  aca->current_data = allocate_clck_memory (initial_capacity, elem_size, &aca->current_occupied, e);
  if (aca->current_data == NULL)
    {
      i_free (aca);
      return e->cause_code;
    }

  aca->current_capacity = initial_capacity;
  aca->current_clock = 0;

  // No previous allocator initially
  aca->prev_data = NULL;
  aca->prev_occupied = NULL;
  aca->prev_capacity = 0;

  // Allocate handle table
  aca->handle_table = i_malloc (initial_capacity, sizeof (struct clock_frame), e);
  if (aca->handle_table == NULL)
    {
      i_free (aca->current_data);
      i_free (aca);
      return e->cause_code;
    }

  aca->handle_capacity = initial_capacity;
  for (u32 i = 0; i < initial_capacity; ++i)
    {
      aca->handle_table[i].allocated = false;
    }

  spx_latch_init (&aca->latch);

  return SUCCESS;
}

i32
adptv_clck_alloc_alloc (struct adptv_clck_alloc *aca, error *e)
{
  ASSERT (aca);

  spx_latch_lock_x (&aca->latch);

  // Finish migration if in progress (must complete before new allocations)
  if (aca->prev_data != NULL)
    {
      finish_migration (aca);
    }

  // Check if we need to expand (size == capacity after this allocation)
  if (aca->size >= aca->current_capacity)
    {
      u32 new_capacity = aca->current_capacity * 2;
      if (new_capacity <= aca->settings.max_capacity)
        {
          if (trigger_resize (aca, new_capacity, e) != SUCCESS)
            {
              spx_latch_unlock_x (&aca->latch);
              return -1;
            }
        }
      else
        {
          spx_latch_unlock_x (&aca->latch);
          error_causef (e, ERR_NOMEM, "Allocator at max capacity");
          return -1;
        }
    }

  // Find free handle
  i32 handle = -1;
  for (u32 i = 0; i < aca->handle_capacity; ++i)
    {
      if (!aca->handle_table[i].allocated)
        {
          handle = i;
          break;
        }
    }

  if (handle == -1)
    {
      spx_latch_unlock_x (&aca->latch);
      error_causef (e, ERR_NOMEM, "No free handles");
      return -1;
    }

  // Find free slot in current allocator
  i32 phys_idx = find_free_slot (aca->current_data, aca->current_occupied, &aca->current_clock, aca->current_capacity);
  if (phys_idx == -1)
    {
      spx_latch_unlock_x (&aca->latch);
      error_causef (e, ERR_NOMEM, "No free slots in current allocator");
      return -1;
    }

  // Allocate
  aca->current_occupied[phys_idx] = true;
  aca->handle_table[handle].allocated = true;
  aca->handle_table[handle].in_current = true;
  aca->handle_table[handle].phys_idx = phys_idx;
  aca->size++;

  spx_latch_unlock_x (&aca->latch);

  return handle;
}

i32
adptv_clck_alloc_calloc (struct adptv_clck_alloc *aca, error *e)
{
  i32 handle = adptv_clck_alloc_alloc (aca, e);
  if (handle < 0)
    {
      return handle;
    }

  void *ptr = adptv_clck_alloc_get_at (aca, handle);
  if (ptr)
    {
      i_memset (ptr, 0, aca->elem_size);
    }

  return handle;
}

void
adptv_clck_alloc_free (struct adptv_clck_alloc *aca, i32 handle,
                       error *e)
{
  ASSERT (aca);
  ASSERT (handle >= 0 && (u32)handle < aca->handle_capacity);

  spx_latch_lock_x (&aca->latch);

  if (!aca->handle_table[handle].allocated)
    {
      spx_latch_unlock_x (&aca->latch);
      error_causef (e, ERR_INVALID_ARGUMENT, "Handle %d not allocated", handle);
      return;
    }

  // Free the physical slot
  if (aca->handle_table[handle].in_current)
    {
      u32 idx = aca->handle_table[handle].phys_idx;
      aca->current_occupied[idx] = false;
    }
  else
    {
      u32 idx = aca->handle_table[handle].phys_idx;
      aca->prev_occupied[idx] = false;
    }

  // Mark handle as free
  aca->handle_table[handle].allocated = false;
  aca->size--;

  // Check if we need to shrink (size <= capacity * 0.25)
  u32 threshold = aca->current_capacity / 4;
  if (aca->size <= threshold && aca->size > 0)
    {
      u32 new_capacity = aca->current_capacity / 2;
      if (new_capacity >= aca->settings.min_capacity)
        {
          trigger_resize (aca, new_capacity, e);
        }
    }

  spx_latch_unlock_x (&aca->latch);
}

void *
adptv_clck_alloc_get_at (struct adptv_clck_alloc *aca, i32 handle)
{
  ASSERT (aca);

  if (handle < 0 || (u32)handle >= aca->handle_capacity)
    {
      return NULL;
    }

  if (!aca->handle_table[handle].allocated)
    {
      return NULL;
    }

  u32 idx = aca->handle_table[handle].phys_idx;

  if (aca->handle_table[handle].in_current)
    {
      return get_element_ptr (aca->current_data, idx, aca->elem_size);
    }
  else
    {
      return get_element_ptr (aca->prev_data, idx, aca->elem_size);
    }
}

u32
adptv_clck_alloc_size (const struct adptv_clck_alloc *aca)
{
  ASSERT (aca);
  return aca->size;
}

u32
adptv_clck_alloc_capacity (const struct adptv_clck_alloc *aca)
{
  ASSERT (aca);
  return aca->current_capacity;
}

void
adptv_clck_alloc_close (struct adptv_clck_alloc *aca)
{
  ASSERT (aca);

  if (aca->current_data)
    {
      i_free (aca->current_data);
    }

  if (aca->prev_data)
    {
      i_free (aca->prev_data);
    }

  if (aca->handle_table)
    {
      i_free (aca->handle_table);
    }
}

#include <numstore/test/testing.h>

TEST (TT_UNIT, adptv_clck_alloc_basic)
{
  error e = error_create ();
  struct adptv_clck_alloc aca;
  struct adptv_clck_alloc_settings settings = {
    .migration_work = 4,
    .max_capacity = 1024,
    .min_capacity = 4
  };

  test_err_t_wrap (adptv_clck_alloc_init (&aca, sizeof (i32), 8, settings, &e), &e);
  test_assert_int_equal (adptv_clck_alloc_size (&aca), 0);
  test_assert_int_equal (adptv_clck_alloc_capacity (&aca), 8);

  // Allocate and write
  i32 h1 = adptv_clck_alloc_alloc (&aca, &e);
  test_err_t_wrap (e.cause_code, &e);
  test_assert (h1 >= 0);

  i32 *p1 = (i32 *)adptv_clck_alloc_get_at (&aca, h1);
  *p1 = 42;

  i32 h2 = adptv_clck_alloc_calloc (&aca, &e);
  test_err_t_wrap (e.cause_code, &e);
  i32 *p2 = (i32 *)adptv_clck_alloc_get_at (&aca, h2);
  test_assert_int_equal (*p2, 0);
  *p2 = 100;

  test_assert_int_equal (*((i32 *)adptv_clck_alloc_get_at (&aca, h1)), 42);
  test_assert_int_equal (*((i32 *)adptv_clck_alloc_get_at (&aca, h2)), 100);

  // Free and verify
  adptv_clck_alloc_free (&aca, h1, &e);
  test_assert (adptv_clck_alloc_get_at (&aca, h1) == NULL);

  adptv_clck_alloc_close (&aca);
}

TEST (TT_UNIT, adptv_clck_alloc_resize_up)
{
  error e = error_create ();
  struct adptv_clck_alloc aca;
  struct adptv_clck_alloc_settings settings = {
    .migration_work = 2,
    .max_capacity = 256,
    .min_capacity = 4
  };

  test_err_t_wrap (adptv_clck_alloc_init (&aca, sizeof (u32), 4, settings, &e), &e);

  i32 handles[10];
  // Allocate 4 elements
  for (u32 i = 0; i < 4; ++i)
    {
      handles[i] = adptv_clck_alloc_alloc (&aca, &e);
      test_err_t_wrap (e.cause_code, &e);
      u32 *ptr = (u32 *)adptv_clck_alloc_get_at (&aca, handles[i]);
      *ptr = i * 10;
    }

  test_assert_int_equal (adptv_clck_alloc_capacity (&aca), 4);

  // Verify before resize
  for (u32 i = 0; i < 4; ++i)
    {
      u32 *p = (u32 *)adptv_clck_alloc_get_at (&aca, handles[i]);
      test_assert_int_equal (*p, i * 10);
    }

  // Trigger expansion
  handles[4] = adptv_clck_alloc_alloc (&aca, &e);
  test_err_t_wrap (e.cause_code, &e);
  test_assert_int_equal (adptv_clck_alloc_capacity (&aca), 8);

  // Verify old values after resize
  for (u32 i = 0; i < 4; ++i)
    {
      u32 *p = (u32 *)adptv_clck_alloc_get_at (&aca, handles[i]);
      test_assert_int_equal (*p, i * 10);
    }

  u32 *ptr = (u32 *)adptv_clck_alloc_get_at (&aca, handles[4]);
  *ptr = 40;

  // Verify all 5 values
  for (u32 i = 0; i < 5; ++i)
    {
      u32 *p = (u32 *)adptv_clck_alloc_get_at (&aca, handles[i]);
      test_assert_int_equal (*p, i * 10);
    }

  adptv_clck_alloc_close (&aca);
}

TEST (TT_UNIT, adptv_clck_alloc_resize_down)
{
  error e = error_create ();
  struct adptv_clck_alloc aca;
  struct adptv_clck_alloc_settings settings = {
    .migration_work = 4,
    .max_capacity = 256,
    .min_capacity = 4
  };

  test_err_t_wrap (adptv_clck_alloc_init (&aca, sizeof (u64), 16, settings, &e), &e);

  i32 handles[17];
  for (u32 i = 0; i < 16; ++i)
    {
      handles[i] = adptv_clck_alloc_alloc (&aca, &e);
      test_err_t_wrap (e.cause_code, &e);
      u64 *ptr = (u64 *)adptv_clck_alloc_get_at (&aca, handles[i]);
      *ptr = i * 1000;
    }

  // Trigger expansion to 32
  handles[16] = adptv_clck_alloc_alloc (&aca, &e);
  test_err_t_wrap (e.cause_code, &e);
  u64 *ptr = (u64 *)adptv_clck_alloc_get_at (&aca, handles[16]);
  *ptr = 16000;
  test_assert_int_equal (adptv_clck_alloc_capacity (&aca), 32);

  // Free to trigger shrinkage
  for (u32 i = 0; i < 9; ++i)
    {
      adptv_clck_alloc_free (&aca, handles[i], &e);
      test_err_t_wrap (e.cause_code, &e);
    }

  test_assert_int_equal (adptv_clck_alloc_size (&aca), 8);
  test_assert_int_equal (adptv_clck_alloc_capacity (&aca), 16);

  // Verify remaining values
  for (u32 i = 9; i < 17; ++i)
    {
      u64 *p = (u64 *)adptv_clck_alloc_get_at (&aca, handles[i]);
      test_fail_if_null (p);
      test_assert_int_equal (*p, i * 1000);
    }

  adptv_clck_alloc_close (&aca);
}
