#include "type/builders/enum.h"
#include "dev/assert.h"
#include "ds/strings.h"
#include "errors/error.h"
#include "intf/mm.h"
#include "type/types.h"

DEFINE_DBG_ASSERT_I (enum_builder, enum_builder, s)
{
  ASSERT (s);
  ASSERT (s->cap > 0);
  ASSERT (s->len <= s->cap);
  ASSERT (s->keys);
}

err_t
enb_create (enum_builder *dest, lalloc *alloc, error *e)
{
  ASSERT (dest);

  lalloc_r keys = lmalloc (alloc, 10, 2, sizeof *dest->keys);
  if (keys.stat != AR_SUCCESS)
    {
      return error_causef (
          e, ERR_NOMEM,
          "Enum Builder: "
          "Failed to allocate memory for enum keys buffer");
    }

  dest->keys = keys.ret;
  dest->len = 0;
  dest->cap = keys.rlen;
  dest->alloc = alloc;

  enum_builder_assert (dest);

  return SUCCESS;
}

#define MAX(a, b) ((a) > (b) ? (a) : (b))

static inline err_t
enb_expand (enum_builder *eb, error *e)
{
  enum_builder_assert (eb);

  u32 cap = 2 * eb->cap;

  lalloc_r keys = lrealloc (
      eb->alloc,
      eb->keys,
      2 * eb->cap,
      eb->cap + 1,
      sizeof *eb->keys);

  if (keys.stat != AR_SUCCESS)
    {
      return error_causef (
          e, ERR_NOMEM,
          "Enum Builder: "
          "Failed to expand enum keys buffer");
    }

  eb->cap = cap;
  eb->keys = keys.ret;

  return SUCCESS;
}

static inline bool
enb_has_key_been_used (const enum_builder *eb, const string key)
{
  for (u32 i = 0; i < eb->len; ++i)
    {
      if (string_equal (eb->keys[i], key))
        {
          return true;
        }
    }
  return false;
}

err_t
enb_accept_key (enum_builder *eb, string key, error *e)
{
  enum_builder_assert (eb);

  if (enb_has_key_been_used (eb, key))
    {
      return error_causef (
          e, ERR_INVALID_TYPE,
          "Enum Builder: "
          "Key: %.*s has already been used",
          key.len, key.data);
    }

  if (eb->len == eb->cap)
    {
      err_t_wrap (enb_expand (eb, e), e);
    }

  eb->keys[eb->len++] = key;

  return SUCCESS;
}

static inline err_t
enb_clip (enum_builder *eb, error *e)
{
  enum_builder_assert (eb);

  if (eb->len != eb->cap)
    {
      lalloc_r keys = lrealloc (
          eb->alloc,
          eb->keys,
          eb->len,
          eb->len,
          sizeof *eb->keys);

      if (keys.stat != AR_SUCCESS)
        {
          /*
           * Edge condition realloc shrink fail, treat as nomem
           */
          return error_causef (
              e, ERR_NOMEM,
              "Enum Builder: "
              "Failed to shrink enum keys buffer");
        }

      eb->cap = eb->len;
      eb->keys = keys.ret;
    }

  return SUCCESS;
}

err_t
enb_build (enum_t *dest, enum_builder *eb, error *e)
{
  enum_builder_assert (eb);
  ASSERT (dest);

  if (eb->len == 0)
    {
      return error_causef (
          e, ERR_INVALID_TYPE,
          "Enum Builder: "
          "Expecting at least 1 type");
    }

  err_t_wrap (enb_clip (eb, e), e);

  dest->keys = eb->keys;
  dest->len = eb->len;

  return SUCCESS;
}
