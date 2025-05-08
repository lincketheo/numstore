#include "type/builders/enum.h"
#include "dev/assert.h"
#include "dev/errors.h"
#include "ds/strings.h"
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
enb_create (enum_builder *dest, lalloc *alloc)
{
  ASSERT (dest);

  string *keys = lmalloc (alloc, 10 * sizeof *keys);
  if (!keys)
    {
      return ERR_NOMEM;
    }

  dest->keys = keys;
  dest->len = 0;
  dest->cap = 10;
  dest->alloc = alloc;

  enum_builder_assert (dest);

  return SUCCESS;
}

#define MAX(a, b) ((a) > (b) ? (a) : (b))

static inline err_t
enb_double_cap (enum_builder *eb)
{
  enum_builder_assert (eb);

  u32 cap = 2 * eb->cap;

  string *keys = lrealloc (eb->alloc, eb->keys, cap * sizeof *keys);
  if (!keys)
    {
      return ERR_NOMEM;
    }
  eb->cap = cap;
  eb->keys = keys;

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
enb_accept_key (enum_builder *eb, string key)
{
  enum_builder_assert (eb);

  err_t ret = SUCCESS;

  if (enb_has_key_been_used (eb, key))
    {
      return ERR_INVALID_ARGUMENT;
    }

  if (eb->len == eb->cap)
    {
      ret = enb_double_cap (eb);
      if (ret)
        {
          return ret;
        }
    }

  eb->keys[eb->len++] = key;

  return SUCCESS;
}

static inline err_t
enb_clip (enum_builder *eb)
{
  enum_builder_assert (eb);

  if (eb->len != eb->cap)
    {
      string *keys = lrealloc (eb->alloc, eb->keys, eb->len * sizeof *keys);
      if (!keys)
        {
          return ERR_IO;
        }
      eb->cap = eb->len;
      eb->keys = keys;
    }

  return SUCCESS;
}

err_t
enb_build (enum_t *dest, enum_builder *eb)
{
  enum_builder_assert (eb);
  ASSERT (dest);

  if (eb->len == 0)
    {
      return ERR_INVALID_ARGUMENT;
    }

  err_t ret = enb_clip (eb);
  if (ret)
    {
      return ret;
    }

  dest->keys = eb->keys;
  dest->len = eb->len;

  return SUCCESS;
}
