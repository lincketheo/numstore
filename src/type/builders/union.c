#include "type/builders/union.h"
#include "dev/assert.h"
#include "ds/strings.h"
#include "errors/error.h"
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
unb_create (union_builder *dest, lalloc *alloc, error *e)
{
  ASSERT (dest);

  lalloc_r keys = lmalloc (
      alloc, 10, 2,
      sizeof *dest->keys);
  if (keys.stat != AR_SUCCESS)
    {
      return error_causef (
          e, ERR_NOMEM,
          "Union Builder: "
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
          "Union Builder: "
          "Not enough memory to allocate types buffer");
    }

  dest->keys = keys.ret;
  dest->klen = 0;
  dest->kcap = keys.rlen;

  dest->types = types.ret;
  dest->tlen = 0;
  dest->tcap = types.rlen;

  dest->alloc = alloc;

  union_builder_assert (dest);

  return SUCCESS;
}

#define MAX(a, b) ((a) > (b) ? (a) : (b))

static inline err_t
unb_expand (union_builder *ub, error *e)
{
  union_builder_assert (ub);

  lalloc_r keys = lrealloc (
      ub->alloc,
      ub->keys,
      2 * ub->kcap, ub->kcap + 1,
      sizeof *ub->keys);
  if (keys.stat != AR_SUCCESS)
    {
      return error_causef (
          e, ERR_NOMEM,
          "Union Builder: "
          "Failed to reallocate keys buffer");
    }
  ub->kcap = keys.rlen;
  ub->keys = keys.ret;

  lalloc_r types = lrealloc (
      ub->alloc,
      ub->types,
      ub->kcap, ub->kcap,
      sizeof *ub->types);
  if (types.stat != AR_SUCCESS)
    {
      return error_causef (
          e, ERR_NOMEM,
          "Union Builder: "
          "Failed to reallocate keys buffer");
    }
  ub->tcap = types.rlen;
  ub->types = types.ret;

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
unb_accept_key (union_builder *ub, string key, error *e)
{
  union_builder_assert (ub);

  if (unb_has_key_been_used (ub, key))
    {
      return error_causef (
          e, ERR_INVALID_TYPE,
          "Union Builder: "
          "Key: %.*s has already been used",
          key.len, key.data);
    }

  if (ub->klen == ub->kcap)
    {
      err_t_wrap (unb_expand (ub, e), e);
    }

  ub->keys[ub->klen++] = key;

  return SUCCESS;
}

err_t
unb_accept_type (union_builder *ub, type t, error *e)
{
  union_builder_assert (ub);

  if (ub->tlen == ub->tcap)
    {
      err_t_wrap (unb_expand (ub, e), e);
    }

  ub->types[ub->tlen++] = t;

  return SUCCESS;
}

static inline err_t
unb_clip (union_builder *ub, error *e)
{
  union_builder_assert (ub);

  if (ub->klen != ub->kcap)
    {
      lalloc_r keys = lrealloc (
          ub->alloc,
          ub->keys,
          ub->klen, ub->klen,
          sizeof *ub->keys);
      if (keys.stat != AR_SUCCESS)
        {
          /*
           * Edge condition realloc shrink fail, treat as nomem
           */
          return error_causef (
              e, ERR_NOMEM,
              "Union Builder: "
              "Failed to shrink keys buffer");
        }

      ub->kcap = keys.rlen;
      ub->keys = keys.ret;
    }

  if (ub->tlen != ub->tcap)
    {
      lalloc_r types = lrealloc (
          ub->alloc,
          ub->types,
          ub->tlen, ub->tlen,
          sizeof *ub->types);
      if (types.stat != AR_SUCCESS)
        {
          /*
           * Edge condition realloc shrink fail, treat as nomem
           */
          return error_causef (
              e, ERR_NOMEM,
              "Union Builder: "
              "Failed to shrink types buffer");
        }

      ub->tcap = types.rlen;
      ub->types = types.ret;
    }

  return SUCCESS;
}

err_t
unb_build (union_t *dest, union_builder *ub, error *e)
{
  union_builder_assert (ub);
  ASSERT (dest);

  if (ub->tlen == 0)
    {
      return error_causef (
          e, ERR_INVALID_TYPE,
          "Union Builder: "
          "Expecting at least one key");
    }
  if (ub->tlen != ub->klen)
    {
      return error_causef (
          e, ERR_INVALID_TYPE,
          "Union Builder: "
          "Expects to have the same number of keys as values");
    }

  err_t_wrap (unb_clip (ub, e), e);

  dest->keys = ub->keys;
  dest->types = ub->types;
  dest->len = ub->tlen;

  return SUCCESS;
}
