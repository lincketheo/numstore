#include "type/builders/struct.h"
#include "dev/assert.h"
#include "ds/strings.h"
#include "errors/error.h"
#include "mm/lalloc.h"
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
stb_create (struct_builder *dest, lalloc *alloc, error *e)
{
  ASSERT (dest);

  lalloc_r keys = lmalloc (
      alloc, 10, 2,
      sizeof *dest->keys);
  if (keys.stat != AR_SUCCESS)
    {
      return error_causef (
          e, ERR_NOMEM,
          "Struct Builder: "
          "Not enough memory to allocate keys buffer");
    }

  lalloc_r types = lmalloc (
      alloc,
      keys.rlen, keys.rlen,
      sizeof *dest->types);
  if (types.stat != AR_SUCCESS)
    {
      return error_causef (
          e, ERR_NOMEM,
          "Struct Builder: "
          "Not enough memory to allocate types buffer");
    }

  dest->keys = keys.ret;
  dest->klen = 0;
  dest->kcap = keys.rlen;

  dest->types = types.ret;
  dest->tlen = 0;
  dest->tcap = types.rlen;

  dest->alloc = alloc;

  struct_builder_assert (dest);

  return SUCCESS;
}

#define MAX(a, b) ((a) > (b) ? (a) : (b))

static inline err_t
stb_expand (struct_builder *sb, error *e)
{
  struct_builder_assert (sb);

  lalloc_r keys = lrealloc (
      sb->alloc,
      sb->keys,
      2 * sb->kcap, sb->kcap + 1,
      sizeof *sb->keys);
  if (keys.stat != AR_SUCCESS)
    {
      return error_causef (
          e, ERR_NOMEM,
          "Struct Builder: "
          "Failed to reallocate keys buffer");
    }
  sb->kcap = keys.rlen;
  sb->keys = keys.ret;

  lalloc_r types = lrealloc (
      sb->alloc,
      sb->types,
      sb->kcap, sb->kcap,
      sizeof *sb->types);
  if (types.stat != AR_SUCCESS)
    {
      return error_causef (
          e, ERR_NOMEM,
          "Struct Builder: "
          "Failed to reallocate keys buffer");
    }
  sb->tcap = types.rlen;
  sb->types = types.ret;

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
stb_accept_key (struct_builder *sb, string key, error *e)
{
  struct_builder_assert (sb);

  if (stb_has_key_been_used (sb, key))
    {
      return error_causef (
          e, ERR_INVALID_TYPE,
          "Struct Builder: "
          "Key: %.*s has already been used",
          key.len, key.data);
    }

  if (sb->klen == sb->kcap)
    {
      err_t_wrap (stb_expand (sb, e), e);
    }

  sb->keys[sb->klen++] = key;

  return SUCCESS;
}

err_t
stb_accept_type (struct_builder *sb, type t, error *e)
{
  struct_builder_assert (sb);

  if (sb->tlen == sb->tcap)
    {
      err_t_wrap (stb_expand (sb, e), e);
    }

  sb->types[sb->tlen++] = t;

  return SUCCESS;
}

static inline err_t
stb_clip (struct_builder *sb, error *e)
{
  struct_builder_assert (sb);

  if (sb->klen != sb->kcap)
    {
      lalloc_r keys = lrealloc (
          sb->alloc,
          sb->keys,
          sb->klen, sb->klen,
          sizeof *sb->keys);
      if (keys.stat != AR_SUCCESS)
        {
          /*
           * Edge condition realloc shrink fail, treat as nomem
           */
          return error_causef (
              e, ERR_NOMEM,
              "Struct Builder: "
              "Failed to shrink keys buffer");
        }

      sb->kcap = keys.rlen;
      sb->keys = keys.ret;
    }

  if (sb->tlen != sb->tcap)
    {
      lalloc_r types = lrealloc (
          sb->alloc,
          sb->types,
          sb->tlen, sb->tlen,
          sizeof *sb->types);
      if (types.stat != AR_SUCCESS)
        {
          /*
           * Edge condition realloc shrink fail, treat as nomem
           */
          return error_causef (
              e, ERR_NOMEM,
              "Struct Builder: "
              "Failed to shrink types buffer");
        }

      sb->tcap = types.rlen;
      sb->types = types.ret;
    }

  return SUCCESS;
}

err_t
stb_build (struct_t *dest, struct_builder *sb, error *e)
{
  struct_builder_assert (sb);
  ASSERT (dest);

  if (sb->tlen == 0)
    {
      return error_causef (
          e, ERR_INVALID_TYPE,
          "Struct Builder: "
          "Expecting at least one key");
    }
  if (sb->tlen != sb->klen)
    {
      return error_causef (
          e, ERR_INVALID_TYPE,
          "Struct Builder: "
          "Expects to have the same number of keys as values");
    }

  err_t_wrap (stb_clip (sb, e), e);

  dest->keys = sb->keys;
  dest->types = sb->types;
  dest->len = sb->tlen;

  return SUCCESS;
}
