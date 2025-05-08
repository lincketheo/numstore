#include "type/builders/struct.h"
#include "dev/assert.h"
#include "dev/errors.h"
#include "ds/strings.h"
#include "intf/mm.h"
#include "type/types.h"

DEFINE_DBG_ASSERT_I (struct_builder, struct_builder, s)
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
stb_create (struct_builder *dest, lalloc *alloc)
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

  struct_builder_assert (dest);

  return SUCCESS;
}

#define MAX(a, b) ((a) > (b) ? (a) : (b))

static inline err_t
stb_double_cap (struct_builder *sb)
{
  struct_builder_assert (sb);

  u32 cap = 2 * MAX (sb->kcap, sb->tcap);

  string *keys = lrealloc (sb->alloc, sb->keys, cap * sizeof *keys);
  if (!keys)
    {
      return ERR_NOMEM;
    }
  sb->kcap = cap;
  sb->keys = keys;

  type *types = lrealloc (sb->alloc, sb->types, cap * sizeof *types);
  if (!types)
    {
      return ERR_NOMEM;
    }
  sb->tcap = cap;
  sb->types = types;

  return SUCCESS;
}

static inline bool
stb_has_key_been_used (const struct_builder *sb, const string key)
{
  for (u32 i = 0; i < sb->klen; ++i)
    {
      if (string_equal (sb->keys[i], key))
        {
          return true;
        }
    }
  return false;
}

err_t
stb_accept_key (struct_builder *sb, string key)
{
  struct_builder_assert (sb);

  err_t ret = SUCCESS;

  if (stb_has_key_been_used (sb, key))
    {
      return ERR_INVALID_ARGUMENT;
    }

  if (sb->klen == sb->kcap)
    {
      ret = stb_double_cap (sb);
      if (ret)
        {
          return ret;
        }
    }

  sb->keys[sb->klen++] = key;

  return SUCCESS;
}

err_t
stb_accept_type (struct_builder *sb, type t)
{
  struct_builder_assert (sb);

  err_t ret = SUCCESS;

  if (sb->tlen == sb->tcap)
    {
      ret = stb_double_cap (sb);
      if (ret)
        {
          return ret;
        }
    }

  sb->types[sb->tlen++] = t;

  return SUCCESS;
}

static inline err_t
stb_clip (struct_builder *sb)
{
  struct_builder_assert (sb);

  if (sb->klen != sb->kcap)
    {
      string *keys = lrealloc (sb->alloc, sb->keys, sb->klen * sizeof *keys);
      if (!keys)
        {
          return ERR_IO;
        }
      sb->kcap = sb->klen;
      sb->keys = keys;
    }

  if (sb->tlen != sb->tcap)
    {
      type *types = lrealloc (sb->alloc, sb->types, sb->tlen * sizeof *types);
      if (!types)
        {
          return ERR_IO;
        }
      sb->tcap = sb->tlen;
      sb->types = types;
    }

  return SUCCESS;
}

err_t
stb_build (struct_t *dest, struct_builder *sb)
{
  struct_builder_assert (sb);
  ASSERT (dest);

  if (sb->tlen == 0)
    {
      return ERR_INVALID_ARGUMENT;
    }
  if (sb->tlen != sb->klen)
    {
      return ERR_INVALID_ARGUMENT;
    }

  err_t ret = stb_clip (sb);
  if (ret)
    {
      return ret;
    }

  dest->keys = sb->keys;
  dest->types = sb->types;
  dest->len = sb->tlen;

  return SUCCESS;
}
