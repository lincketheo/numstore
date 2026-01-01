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
 *   TODO: Add description for dirty_page_table.c
 */

#include <numstore/pager/dirty_page_table.h>

#include <numstore/core/adptv_hash_table.h>
#include <numstore/core/assert.h>
#include <numstore/core/bytes.h>
#include <numstore/core/clock_allocator.h>
#include <numstore/core/deserializer.h>
#include <numstore/core/error.h>
#include <numstore/core/hash_table.h>
#include <numstore/core/ht_models.h>
#include <numstore/core/random.h>
#include <numstore/core/serializer.h>
#include <numstore/core/spx_latch.h>
#include <numstore/intf/logging.h>
#include <numstore/intf/types.h>
#include <numstore/test/testing.h>

#include <config.h>

DEFINE_DBG_ASSERT (
    struct dpg_table, dirty_pg_table, d, {
      ASSERT (d);
    })

static bool
dpg_entry_equals (const struct hnode *left, const struct hnode *right)
{
  struct dpg_entry *_left = container_of (left, struct dpg_entry, node);
  struct dpg_entry *_right = container_of (right, struct dpg_entry, node);

  spx_latch_lock_s (&_left->l);
  spx_latch_lock_s (&_right->l);

  bool ret = _left->pg == _right->pg;

  spx_latch_unlock_s (&_right->l);
  spx_latch_unlock_s (&_left->l);

  return ret;
}

static void
dpg_entry_key_init (struct dpg_entry *dest, pgno pg)
{
  dest->pg = pg;
  hnode_init (&dest->node, pg);
  spx_latch_init (&dest->l);
}

// Lifecycle
struct dpg_table *
dpgt_open (error *e)
{
  struct dpg_table *ret = i_calloc (1, sizeof *ret, e);
  if (ret == NULL)
    {
      return NULL;
    }

  struct adptv_htable_settings settings = {
    .max_load_factor = 8,
    .min_load_factor = 1,
    .rehashing_work = 28,
    .max_size = 2048,
    .min_size = 10,
  };

  err_t result = adptv_htable_init (&ret->t, settings, e);
  if (result != SUCCESS)
    {
      i_free (ret);
      return NULL;
    }

  result = clck_alloc_open (&ret->alloc, sizeof (struct dpg_entry), 100000, e);
  if (result != SUCCESS)
    {
      adptv_htable_free (&ret->t);
      i_free (ret);
      return NULL;
    }

  spx_latch_init (&ret->l);

  return ret;
}

void
dpgt_close (struct dpg_table *t)
{
  DBG_ASSERT (dirty_pg_table, t);
  spx_latch_lock_x (&t->l);
  adptv_htable_free (&t->t);
  spx_latch_unlock_x (&t->l);
  clck_alloc_close (&t->alloc);
  i_free (t);
}

#ifndef NTEST

struct dpgt_eq_ctx
{
  struct dpg_table *other;
  bool ret;
};

static void
dpgt_eq_foreach (struct hnode *node, void *_ctx)
{
  struct dpgt_eq_ctx *ctx = _ctx;
  if (ctx->ret == false)
    {
      return;
    }

  struct dpg_entry *dpe = container_of (node, struct dpg_entry, node);
  spx_latch_lock_s (&dpe->l);

  struct dpg_entry candidate;
  dpg_entry_key_init (&candidate, dpe->pg);

  struct hnode *other_node = adptv_htable_lookup (&ctx->other->t, &candidate.node, dpg_entry_equals);

  if (other_node == NULL)
    {
      ctx->ret = false;
      spx_latch_unlock_s (&dpe->l);
      return;
    }

  struct dpg_entry *other_dpe = container_of (other_node, struct dpg_entry, node);

  spx_latch_lock_s (&other_dpe->l);

  bool equal = other_dpe->pg == dpe->pg;
  equal = equal && other_dpe->rec_lsn == dpe->rec_lsn;
  ctx->ret = equal;

  spx_latch_unlock_s (&dpe->l);
  spx_latch_unlock_s (&other_dpe->l);
}

bool
dpgt_equal (struct dpg_table *left, struct dpg_table *right)
{
  spx_latch_lock_s (&left->l);
  spx_latch_lock_s (&right->l);

  if (adptv_htable_size (&left->t) != adptv_htable_size (&right->t))
    {
      spx_latch_unlock_s (&right->l);
      spx_latch_unlock_s (&left->l);
      return false;
    }

  struct dpgt_eq_ctx ctx = {
    .other = right,
    .ret = true,
  };
  adptv_htable_foreach (&left->t, dpgt_eq_foreach, &ctx);

  spx_latch_unlock_s (&right->l);
  spx_latch_unlock_s (&left->l);

  return ctx.ret;
}

#endif

void
dpgt_rand_populate (struct dpg_table *dpt)
{
  u32 len = adptv_htable_size (&dpt->t);

  len = randu32r (0, MEMORY_PAGE_LEN - len);

  for (u32 i = 0; i < len; ++i)
    {
      if (dpgt_add (dpt, i, randu32r (0, 100), NULL))
        {
          return;
        }
    }
}

static void
i_log_dpg_entry (struct hnode *node, void *_log_level)
{
  int *log_level = _log_level;
  struct dpg_entry *dpe = container_of (node, struct dpg_entry, node);

  spx_latch_lock_s (&dpe->l);

  i_printf (*log_level, "%" PRpgno " %" PRspgno "\n", dpe->pg, dpe->rec_lsn);

  spx_latch_unlock_s (&dpe->l);
}

void
i_log_dpgt (int log_level, struct dpg_table *dpt)
{
  spx_latch_lock_s (&dpt->l);

  i_log (log_level, "================ Dirty Page Table START ================\n");
  i_printf (log_level, "txid recLSN\n");
  adptv_htable_foreach (&dpt->t, i_log_dpg_entry, &log_level);
  i_log (log_level, "================ Dirty Page Table END ================\n");

  spx_latch_unlock_s (&dpt->l);
}

// Methods
err_t
dpgt_add (struct dpg_table *t, pgno pg, lsn rec_lsn, error *e)
{
  DBG_ASSERT (dirty_pg_table, t);

  spx_latch_lock_x (&t->l);

  struct dpg_entry *v = clck_alloc_alloc (&t->alloc, e);
  if (v == NULL)
    {
      spx_latch_unlock_x (&t->l);
      return error_change_causef (e, ERR_DPGT_FULL, "Not enough space in the dirty page table");
    }

  v->pg = pg;
  v->rec_lsn = rec_lsn;
  spx_latch_init (&v->l);
  hnode_init (&v->node, pg);

  if (adptv_htable_insert (&t->t, &v->node, e))
    {
      clck_alloc_free (&t->alloc, v);
      spx_latch_unlock_x (&t->l);
      return e->cause_code;
    }

  spx_latch_unlock_x (&t->l);

  return SUCCESS;
}

bool
dpe_get (struct dpg_entry *dest, struct dpg_table *t, pgno pg)
{
  DBG_ASSERT (dirty_pg_table, t);

  spx_latch_lock_s (&t->l);

  struct dpg_entry key;
  dpg_entry_key_init (&key, pg);

  struct hnode *node = adptv_htable_lookup (&t->t, &key.node, dpg_entry_equals);
  if (node == NULL)
    {
      spx_latch_unlock_s (&t->l);
      return false;
    }

  struct dpg_entry *dpe = container_of (node, struct dpg_entry, node);
  spx_latch_lock_s (&dpe->l);
  *dest = *dpe;
  spx_latch_unlock_s (&dpe->l);

  spx_latch_unlock_s (&t->l);
  return true;
}

struct dpg_entry
dpgt_get_expect (struct dpg_table *t, pgno pg)
{
  DBG_ASSERT (dirty_pg_table, t);

  spx_latch_lock_s (&t->l);

  struct dpg_entry key;
  dpg_entry_key_init (&key, pg);

  struct hnode *node = adptv_htable_lookup (&t->t, &key.node, dpg_entry_equals);
  ASSERT (node);
  struct dpg_entry *dpe = container_of (node, struct dpg_entry, node);

  spx_latch_lock_s (&dpe->l);
  struct dpg_entry ret = *dpe;
  spx_latch_unlock_s (&dpe->l);

  spx_latch_unlock_s (&t->l);

  return ret;
}

void
dpgt_update (struct dpg_table *d, pgno pg, lsn new_rec_lsn)
{
  spx_latch_lock_s (&d->l);

  struct dpg_entry key;
  dpg_entry_key_init (&key, pg);

  struct hnode *node = adptv_htable_lookup (&d->t, &key.node, dpg_entry_equals);
  ASSERT (node);
  struct dpg_entry *dpe = container_of (node, struct dpg_entry, node);

  spx_latch_lock_x (&dpe->l);
  dpe->rec_lsn = new_rec_lsn;
  spx_latch_unlock_x (&dpe->l);

  spx_latch_unlock_s (&d->l);
}

bool
dpgt_remove (struct dpg_table *t, pgno pg)
{
  DBG_ASSERT (dirty_pg_table, t);

  spx_latch_lock_x (&t->l);

  struct dpg_entry key;
  dpg_entry_key_init (&key, pg);

  struct hnode *node = adptv_htable_lookup (&t->t, &key.node, dpg_entry_equals);

  if (node == NULL)
    {
      spx_latch_unlock_x (&t->l);
      return false;
    }

  struct hnode *deleted = NULL;
  if (adptv_htable_delete (&deleted, &t->t, node, dpg_entry_equals, NULL))
    {
      spx_latch_unlock_x (&t->l);
      return false;
    }

  ASSERT (deleted != NULL);
  struct dpg_entry *dpe = container_of (deleted, struct dpg_entry, node);
  clck_alloc_free (&t->alloc, dpe);

  spx_latch_unlock_x (&t->l);
  return true;
}

void
dpgt_remove_expect (struct dpg_table *t, pgno pg)
{
  spx_latch_lock_x (&t->l);

  struct dpg_entry key;
  dpg_entry_key_init (&key, pg);

  struct hnode *node = adptv_htable_lookup (&t->t, &key.node, dpg_entry_equals);
  ASSERT (node != NULL);

  struct hnode *deleted = NULL;
  err_t result = adptv_htable_delete (&deleted, &t->t, node, dpg_entry_equals, NULL);
  ASSERT (result == SUCCESS);
  ASSERT (deleted != NULL);

  struct dpg_entry *dpe = container_of (deleted, struct dpg_entry, node);
  clck_alloc_free (&t->alloc, dpe);

  spx_latch_unlock_x (&t->l);
}

struct min_rec_lsn_ctx
{
  lsn min;
};

static void
find_min_rec_lsn (struct hnode *node, void *vctx)
{
  struct min_rec_lsn_ctx *ctx = vctx;
  struct dpg_entry *dpe = container_of (node, struct dpg_entry, node);

  spx_latch_lock_s (&dpe->l);
  lsn rec_lsn = dpe->rec_lsn;
  spx_latch_unlock_s (&dpe->l);

  if (rec_lsn < ctx->min)
    {
      ctx->min = rec_lsn;
    }
}

lsn
dpgt_min_rec_lsn (struct dpg_table *d)
{
  struct min_rec_lsn_ctx ctx = { .min = (lsn)-1 };

  spx_latch_lock_s (&d->l);
  adptv_htable_foreach (&d->t, find_min_rec_lsn, &ctx);
  spx_latch_unlock_s (&d->l);

  return ctx.min;
}

struct merge_dpg_ctx
{
  struct dpg_table *dest;
};

static void
merge_dpg_entry (struct hnode *node, void *vctx)
{
  struct merge_dpg_ctx *ctx = vctx;
  struct dpg_entry *src_dpe = container_of (node, struct dpg_entry, node);

  // Try to find existing entry in dest
  struct hnode *dest_node = adptv_htable_lookup (&ctx->dest->t, &src_dpe->node, dpg_entry_equals);

  if (dest_node != NULL)
    {
      // Entry already exists, no need to insert
      return;
    }

  // Entry doesn't exist, insert it
  adptv_htable_insert (&ctx->dest->t, &src_dpe->node, NULL);
}

void
dpgt_merge_into (struct dpg_table *dest, struct dpg_table *src)
{
  struct merge_dpg_ctx ctx = {
    .dest = dest,
  };

  spx_latch_lock_s (&src->l);
  spx_latch_lock_x (&dest->l);

  adptv_htable_foreach (&src->t, merge_dpg_entry, &ctx);

  spx_latch_unlock_x (&dest->l);
  spx_latch_unlock_s (&src->l);
}

//////////////////////////////////////////////////
///// Serialization / Deserialization for checkpoint

struct dpg_serialize_ctx
{
  struct serializer s;
};

static void
dpg_entry_serialize (struct hnode *node, void *vctx)
{
  struct dpg_serialize_ctx *ctx = vctx;
  struct dpg_entry *dpe = container_of (node, struct dpg_entry, node);

  spx_latch_lock_s (&dpe->l);
  pgno pg = dpe->pg;
  lsn l = dpe->rec_lsn;
  spx_latch_unlock_s (&dpe->l);

  // Page
  srlizr_write_expect (&ctx->s, (u8 *)&pg, sizeof (pg));

  // lsn
  srlizr_write_expect (&ctx->s, (u8 *)&l, sizeof (l));
}

u32
dpgt_serialize (u8 dest[MAX_DPGT_SRL_SIZE], struct dpg_table *t)
{
  struct dpg_serialize_ctx ctx = {
    .s = srlizr_create (dest, MAX_DPGT_SRL_SIZE),
  };

  spx_latch_lock_s (&t->l);

  adptv_htable_foreach (&t->t, dpg_entry_serialize, &ctx);

  spx_latch_unlock_s (&t->l);

  return ctx.s.dlen;
}

struct dpg_table *
dpgt_deserialize (const u8 *src, u32 dlen, error *e)
{
  struct dpg_table *t = dpgt_open (e);
  if (t == NULL)
    {
      return NULL;
    }

  if (dlen == 0)
    {
      return t;
    }

  struct deserializer d = dsrlizr_create (src, dlen);

  if (d.dlen > MAX_DPGT_SRL_SIZE)
    {
      error_causef (e, ERR_CORRUPT, "DPG table size: %d is invalid", d.dlen);
      return NULL;
    }

  while (true)
    {
      pgno pg;
      lsn l;

      // Page
      bool more = dsrlizr_read ((u8 *)&pg, sizeof (pg), &d);
      if (!more)
        {
          break;
        }

      // lsn
      dsrlizr_read_expect ((u8 *)&l, sizeof (l), &d);

      dpgt_add (t, pg, l, e);
    }

  return t;
}

#ifndef NTEST
void
dpgt_crash (struct dpg_table *t)
{
  DBG_ASSERT (dirty_pg_table, t);
  adptv_htable_free (&t->t);
  clck_alloc_close (&t->alloc);
  i_free (t);
}
#endif
