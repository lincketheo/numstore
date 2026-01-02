#pragma once

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
 *   A dynamic clock allocator that resizes it's allocation pool when 
 *   it needs more room
 */

#include <numstore/core/error.h>
#include <numstore/core/spx_latch.h>
#include <numstore/intf/types.h>

struct adptv_clck_alloc_settings
{
  u32 migration_work;
  u32 max_capacity;
  u32 min_capacity;
};

#define ADPTV_CLCK_ALLOC_DEFAULT_SETTINGS                           \
  (struct adptv_clck_alloc_settings)                                \
  {                                                                 \
    .migration_work = 4, .max_capacity = 1 << 20, /* 1M elements */ \
        .min_capacity = 16,                                         \
  }

struct clock_frame
{
  bool allocated;  // Is this handle currently allocated?
  bool in_current; // True if data is in current allocator, false if in prev
  u32 phys_idx;    // Physical index within the allocator
};

struct adptv_clck_alloc
{
  void *current_data;
  bool *current_occupied;
  u32 current_capacity;
  u32 current_clock;

  void *prev_data;
  bool *prev_occupied;
  u32 prev_capacity;

  struct clock_frame *handle_table;
  u32 handle_capacity;

  u32 size;
  size_t elem_size;
  u32 migrate_pos;
  struct adptv_clck_alloc_settings settings;
  struct spx_latch latch;
};

int adptv_clck_alloc_init (
    struct adptv_clck_alloc *dest,
    size_t elem_size,
    u32 initial_capacity,
    struct adptv_clck_alloc_settings settings,
    error *e);
void adptv_clck_alloc_close (struct adptv_clck_alloc *aca);

i32 adptv_clck_alloc_alloc (struct adptv_clck_alloc *aca, error *e);
i32 adptv_clck_alloc_calloc (struct adptv_clck_alloc *aca, error *e);
void adptv_clck_alloc_free (struct adptv_clck_alloc *aca, i32 handle, error *e);

void *adptv_clck_alloc_get_at (struct adptv_clck_alloc *aca, i32 handle);

// Utils
u32 adptv_clck_alloc_size (const struct adptv_clck_alloc *aca);
u32 adptv_clck_alloc_capacity (const struct adptv_clck_alloc *aca);
