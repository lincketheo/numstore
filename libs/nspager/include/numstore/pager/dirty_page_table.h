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
 *   TODO: Add description for dirty_page_table.h
 */

#include <numstore/core/adptv_hash_table.h>
#include <numstore/core/bytes.h>
#include <numstore/core/clock_allocator.h>
#include <numstore/core/error.h>
#include <numstore/core/hash_table.h>
#include <numstore/intf/types.h>

#include <config.h>

struct dpg_entry
{
  pgno pg;
  lsn rec_lsn;
  struct spx_latch l;
  struct hnode node;
};

struct dpg_table
{
  struct adptv_htable t;
  struct clck_alloc alloc;
  struct spx_latch l;
};

// LIFECYCLE
struct dpg_table *dpgt_open (error *e);
void dpgt_close (struct dpg_table *t);

// UTILS
void dpgt_rand_populate (struct dpg_table *dpt);
void i_log_dpgt (int log_level, struct dpg_table *dpt);
lsn dpgt_min_rec_lsn (struct dpg_table *d);
void dpgt_merge_into (struct dpg_table *dest, struct dpg_table *src);

// INSERT
err_t dpgt_add (struct dpg_table *t, pgno pg, lsn rec_lsn, error *e);

// GET
bool dpe_get (struct dpg_entry *dest, struct dpg_table *t, pgno pg);
struct dpg_entry dpgt_get_expect (struct dpg_table *t, pgno pg);

// REMOVE
bool dpgt_remove (struct dpg_table *t, pgno pg);
void dpgt_remove_expect (struct dpg_table *t, pgno pg);

// UPDATE
void dpgt_update (struct dpg_table *d, pgno pg, lsn new_rec_lsn);

// SERIALIZATION
#define MAX_DPGT_SRL_SIZE (MEMORY_PAGE_LEN * (sizeof (pgno) + sizeof (lsn)))
u32 dpgt_serialize (u8 dest[MAX_DPGT_SRL_SIZE], struct dpg_table *t);
struct dpg_table *dpgt_deserialize (const u8 *src, u32 dlen, error *e);

#ifndef NTEST
void dpgt_crash (struct dpg_table *t);
bool dpgt_equal (struct dpg_table *left, struct dpg_table *right);
#endif
