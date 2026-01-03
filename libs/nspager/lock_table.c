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
 *   TODO: Add description for lock_table.c
 */

#include "numstore/pager/lt_lock.h"
#include "numstore/pager/txn.h"
#include <numstore/pager/lock_table.h>

#include <numstore/core/adptv_hash_table.h>
#include <numstore/core/assert.h>
#include <numstore/core/clock_allocator.h>
#include <numstore/core/error.h>
#include <numstore/core/gr_lock.h>
#include <numstore/core/hash_table.h>
#include <numstore/core/hashing.h>
#include <numstore/core/ht_models.h>
#include <numstore/core/spx_latch.h>

static void
lt_lock_init_key_from_txn (struct lt_lock *dest)
{
  ASSERT (dest);

  char hcode[sizeof (dest->data) + sizeof (u8)];
  hcode[0] = dest->type;
  u32 hcodelen = 1;

  switch (dest->type)
    {
    case LOCK_DB:
    case LOCK_ROOT:
    case LOCK_FSTMBST:
    case LOCK_MSLSN:
    case LOCK_VHP:
    case LOCK_VHPOS:
      {
        hcodelen += i_memcpy (&hcode[hcodelen], &dest->data.vhpos, sizeof (dest->data.vhpos));
        break;
      }
    case LOCK_VAR:
      {
        hcodelen += i_memcpy (&hcode[hcodelen], &dest->data.vhpos, sizeof (dest->data.vhpos));
        break;
      }
    case LOCK_VAR_NEXT:
      {
        hcodelen += i_memcpy (&hcode[hcodelen], &dest->data.var_root_next, sizeof (dest->data.var_root_next));
        break;
      }
    case LOCK_RPTREE:
      {
        hcodelen += i_memcpy (&hcode[hcodelen], &dest->data.rptree_root, sizeof (dest->data.rptree_root));
        break;
      }
    case LOCK_TMBST:
      {
        hcodelen += i_memcpy (&hcode[hcodelen], &dest->data.tmbst_pg, sizeof (dest->data.tmbst_pg));
        break;
      }
    }

  dest->type = dest->type;
  dest->data = dest->data;

  struct cstring lock_type_hcode = {
    .data = hcode,
    .len = hcodelen,
  };

  hnode_init (&dest->lock_type_node, fnv1a_hash (lock_type_hcode));
}

static void
lt_lock_init_key (struct lt_lock *dest, enum lt_lock_type type, union lt_lock_data data)
{
  ASSERT (dest);
  dest->type = type;
  dest->data = data;
  lt_lock_init_key_from_txn (dest);
}

static bool
lt_lock_eq (const struct hnode *left, const struct hnode *right)
{
  const struct lt_lock *_left = container_of (left, struct lt_lock, lock_type_node);
  const struct lt_lock *_right = container_of (right, struct lt_lock, lock_type_node);

  if (_left->type != _right->type)
    {
      return false;
    }

  switch (_left->type)
    {
    case LOCK_DB:
      {
        return true;
      }
    case LOCK_ROOT:
      {
        return true;
      }
    case LOCK_FSTMBST:
      {
        return true;
      }
    case LOCK_MSLSN:
      {
        return true;
      }
    case LOCK_VHP:
      {
        return true;
      }
    case LOCK_VHPOS:
      {
        return _left->data.vhpos == _right->data.vhpos;
      }
    case LOCK_VAR:
      {
        return _left->data.var_root == _right->data.var_root;
      }
    case LOCK_VAR_NEXT:
      {
        return _left->data.var_root_next == _right->data.var_root_next;
      }
    case LOCK_RPTREE:
      {
        return _left->data.rptree_root == _right->data.rptree_root;
      }
    case LOCK_TMBST:
      {
        return _left->data.tmbst_pg == _right->data.tmbst_pg;
      }
    }
  UNREACHABLE ();
}

err_t
lockt_init (struct lockt *t, error *e)
{
  if (clck_alloc_open (&t->gr_lock_alloc, sizeof (struct gr_lock), 1000, e))
    {
      return e->cause_code;
    }

  struct adptv_htable_settings settings = {
    .max_load_factor = 8,
    .min_load_factor = 1,
    .rehashing_work = 28,
    .max_size = 2048,
    .min_size = 10,
  };

  if (adptv_htable_init (&t->table, settings, e))
    {
      clck_alloc_close (&t->gr_lock_alloc);
      return e->cause_code;
    }

  spx_latch_init (&t->l);

  return SUCCESS;
}

void
lockt_destroy (struct lockt *t)
{
  // TODO - wait for all locks?
  clck_alloc_close (&t->gr_lock_alloc);
  adptv_htable_free (&t->table);
}

static struct lt_lock *
lockt_lock_once (
    struct lockt *t,
    enum lt_lock_type type,
    union lt_lock_data data,
    enum lock_mode mode,
    struct txn *tx,
    error *e)
{
  struct lt_lock *lock = txn_newlock (tx, type, data, mode, e);
  if (lock == NULL)
    {
      return NULL;
    }

  // Initialize the lock key
  lt_lock_init_key_from_txn (lock);

  // Lock the hash table
  spx_latch_lock_x (&t->l);

  // Find lock type that intersects this one
  struct hnode *node = adptv_htable_lookup (&t->table, &lock->lock_type_node, lt_lock_eq);

  struct gr_lock *_lock = NULL;

  // FOUND
  if (node != NULL)
    {
      // Lock already exists - try fast path
      struct lt_lock *existing = container_of (node, struct lt_lock, lock_type_node);
      _lock = existing->lock;

      if (!gr_trylock (_lock, mode))
        {
          panic ("TODO - lock coupling");
        }
    }
  else
    {
      // No existing lock - create a new one
      _lock = clck_alloc_alloc (&t->gr_lock_alloc, e);
      if (_lock == NULL)
        {
          spx_latch_unlock_x (&t->l);
          panic ("free lock from txn");
          return NULL;
        }

      if (gr_lock_init (_lock, e))
        {
          clck_alloc_free (&t->gr_lock_alloc, _lock);
          spx_latch_unlock_x (&t->l);
          panic ("free lock from txn");
          return NULL;
        }

      if (gr_lock (_lock, &(struct gr_lock_waiter){ .mode = mode }, e))
        {
          gr_lock_destroy (_lock);
          clck_alloc_free (&t->gr_lock_alloc, _lock);
          spx_latch_unlock_x (&t->l);
          panic ("free lock from txn");
          return NULL;
        }
    }

  // Common path: insert lock into tables
  lock->lock = _lock;

  // Insert into lock type table
  if (adptv_htable_insert (&t->table, &lock->lock_type_node, e))
    {
      gr_unlock (_lock, mode);
      if (node == NULL) // We created this lock
        {
          clck_alloc_free (&t->gr_lock_alloc, _lock);
        }
      spx_latch_unlock_x (&t->l);

      panic ("free txn lock!");

      return NULL;
    }

  spx_latch_unlock_x (&t->l);

  return lock;
}

static const int parent_lock[] = {
  [LOCK_DB] = -1,
  [LOCK_ROOT] = LOCK_DB,
  [LOCK_FSTMBST] = LOCK_ROOT,
  [LOCK_MSLSN] = LOCK_ROOT,
  [LOCK_VHP] = LOCK_DB,
  [LOCK_VHPOS] = LOCK_VHP,
  [LOCK_VAR] = LOCK_DB,
  [LOCK_VAR_NEXT] = LOCK_VAR,
  [LOCK_RPTREE] = LOCK_DB,
  [LOCK_TMBST] = LOCK_DB,
};

static inline enum lock_mode
get_parent_mode (enum lock_mode child_mode)
{
  switch (child_mode)
    {
    case LM_IS:
    case LM_S:
      return LM_IS;
    case LM_IX:
    case LM_SIX:
    case LM_X:
      return LM_IX;
    case LM_COUNT:
      UNREACHABLE ();
    }
  UNREACHABLE ();
}

struct lt_lock *
lockt_lock (
    struct lockt *t,
    enum lt_lock_type type,
    union lt_lock_data data,
    enum lock_mode mode,
    struct txn *tx,
    error *e)
{
  int ptype = parent_lock[type];

  if (ptype != -1)
    {
      enum lock_mode pmode = get_parent_mode (mode);

      struct lt_lock *plock = lockt_lock (t, ptype, data, pmode, tx, e);

      if (plock == NULL)
        {
          return NULL;
        }
    }

  return lockt_lock_once (t, type, data, mode, tx, e);
}

err_t
lockt_upgrade (struct lockt *t, struct lt_lock *lock, enum lock_mode new_mode, error *e)
{
  enum lock_mode old_mode = lock->mode;

  // Check if this is actually an upgrade
  if (new_mode <= old_mode)
    {
      // Already have sufficient lock
      return SUCCESS;
    }

  // Check if we need to upgrade parent locks too
  int ptype = parent_lock[lock->type];

  if (ptype != -1)
    {
      enum lock_mode new_parent_mode = get_parent_mode (new_mode);
      enum lock_mode old_parent_mode = get_parent_mode (old_mode);

      // If parent needs upgrading, upgrade it first
      if (new_parent_mode > old_parent_mode)
        {
          struct lt_lock key;
          lt_lock_init_key (&key, ptype, lock->data);

          struct hnode *_found = adptv_htable_lookup (&t->table, &key.lock_type_node, lt_lock_eq);
          struct lt_lock *found = container_of (_found, struct lt_lock, lock_type_node);
          ASSERT (found);

          err_t ret = lockt_upgrade (t, found, new_parent_mode, e);
          if (ret != SUCCESS)
            {
              return ret;
            }
        }
    }

  if (gr_upgrade (lock->lock, old_mode, &(struct gr_lock_waiter){ .mode = new_mode }, e))
    {
      return e->cause_code;
    }

  lock->mode = new_mode;
  return SUCCESS;
}

err_t
lockt_unlock (struct lockt *t, struct txn *tx, error *e)
{
  ASSERT (t);
  ASSERT (tx);

  spx_latch_lock_x (&t->l);

  // Iterate through all locks and remove them from the main table
  struct lt_lock *curr = tx->locks;

  while (curr != NULL)
    {
      struct lt_lock *next = curr->next;

      // Remove from lock type table
      adptv_htable_delete (NULL, &t->table, &curr->lock_type_node, lt_lock_eq, e);

      // Unlock and check if we should free
      struct gr_lock *gr_lock_ptr = curr->lock;

      bool is_free = gr_unlock (gr_lock_ptr, curr->mode);

      if (is_free)
        {
          gr_lock_destroy (gr_lock_ptr);
          clck_alloc_free (&t->gr_lock_alloc, gr_lock_ptr);
        }

      curr->lock = NULL;
      curr = next;
    }

  txn_free_all_locks (tx);

  spx_latch_unlock_x (&t->l);

  return SUCCESS;
}
