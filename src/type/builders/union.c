#include "type/builders/union.h"
#include "dev/assert.h"
#include "dev/errors.h"
#include "ds/strings.h"
#include "intf/mm.h"
#include "type/types.h"

DEFINE_DBG_ASSERT_I (union_builder, union_builder, s)
{
  ASSERT (s);
  ASSERT (s->kcap > 0);
  ASSERT (s->klen <= s->kcap);

  ASSERT (s->tcap > 0);
  ASSERT (s->tlen <= s->tcap);

  ASSERT (s->keys);
  ASSERT (s->types);
  ASSERT (s->alloc);
}

err_t
unb_create (union_builder *dest, lalloc *alloc)
{
  ASSERT (dest);

  string *keys = lmalloc (alloc, 10 * sizeof *keys);
  if (!keys)
    {
      return ERR_NOMEM;
    }
  type *types = lmalloc (alloc, 10 * sizeof *types);
  if (!types)
    {
      lfree (alloc, keys);
      return ERR_NOMEM;
    }

  dest->keys = keys;
  dest->klen = 0;
  dest->kcap = 10;

  dest->types = types;
  dest->tlen = 0;
  dest->tcap = 10;

  dest->alloc = alloc;

  union_builder_assert (dest);

  return SUCCESS;
}

#define MAX(a, b) ((a) > (b) ? (a) : (b))

static inline err_t
unb_double_cap (union_builder *ub)
{
  union_builder_assert (ub);

  u32 cap = 2 * MAX (ub->kcap, ub->tcap);

  string *keys = lrealloc (ub->alloc, ub->keys, cap * sizeof *keys);
  if (!keys)
    {
      return ERR_NOMEM;
    }
  ub->kcap = cap;
  ub->keys = keys;

  type *types = lrealloc (ub->alloc, ub->types, cap * sizeof *types);
  if (!types)
    {
      return ERR_NOMEM;
    }
  ub->tcap = cap;
  ub->types = types;

  return SUCCESS;
}

static inline bool
unb_has_key_been_used (const union_builder *ub, const string key)
{
  for (u32 i = 0; i < ub->klen; ++i)
    {
      if (string_equal (ub->keys[i], key))
        {
          return true;
        }
    }
  return false;
}

err_t
unb_accept_key (union_builder *ub, string key)
{
  union_builder_assert (ub);

  err_t ret = SUCCESS;

  if (unb_has_key_been_used (ub, key))
    {
      return ERR_INVALID_ARGUMENT;
    }

  if (ub->klen == ub->kcap)
    {
      ret = unb_double_cap (ub);
      if (ret)
        {
          return ret;
        }
    }

  ub->keys[ub->klen++] = key;

  return SUCCESS;
}

err_t
unb_accept_type (union_builder *ub, type t)
{
  union_builder_assert (ub);

  err_t ret = SUCCESS;

  if (ub->tlen == ub->tcap)
    {
      ret = unb_double_cap (ub);
      if (ret)
        {
          return ret;
        }
    }

  ub->types[ub->tlen++] = t;

  return SUCCESS;
}

static inline err_t
unb_clip (union_builder *ub)
{
  union_builder_assert (ub);

  if (ub->klen != ub->kcap)
    {
      string *keys = lrealloc (ub->alloc, ub->keys, ub->klen * sizeof *keys);
      if (!keys)
        {
          return ERR_IO;
        }
      ub->kcap = ub->klen;
      ub->keys = keys;
    }

  if (ub->tlen != ub->tcap)
    {
      type *types = lrealloc (ub->alloc, ub->types, ub->tlen * sizeof *types);
      if (!types)
        {
          return ERR_IO;
        }
      ub->tcap = ub->tlen;
      ub->types = types;
    }

  return SUCCESS;
}

err_t
unb_build (union_t *dest, union_builder *ub)
{
  union_builder_assert (ub);
  ASSERT (dest);

  if (ub->tlen == 0)
    {
      return ERR_INVALID_ARGUMENT;
    }
  if (ub->tlen != ub->klen)
    {
      return ERR_INVALID_ARGUMENT;
    }

  err_t ret = unb_clip (ub);
  if (ret)
    {
      return ret;
    }

  dest->keys = ub->keys;
  dest->types = ub->types;
  dest->len = ub->tlen;

  return SUCCESS;
}
