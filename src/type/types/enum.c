#include "type/types/enum.h"
#include "dev/testing.h"
#include "intf/stdlib.h"
#include "utils/serializer.h"

DEFINE_DBG_ASSERT_I (enum_t, unchecked_enum_t, e)
{
  ASSERT (e);
  ASSERT (e->keys);
}

DEFINE_DBG_ASSERT_I (enum_t, valid_enum_t, e)
{
  ASSERT (enum_t_is_valid (e));
}

bool
enum_t_is_valid (const enum_t *e)
{
  unchecked_enum_t_assert (e);

  if (e->len == 0)
    {
      return false;
    }
  for (u32 i = 0; i < e->len; ++i)
    {
      if (e->keys[i].len == 0)
        {
          return false;
        }
      ASSERT (e->keys[i].data);
    }

  return strings_all_unique (e->keys, e->len);
}

int
enum_t_snprintf (char *str, u32 size, const enum_t *st)
{
  valid_enum_t_assert (st);

  char *out = str;
  u32 avail = size;
  int len = 0;
  int n;

  n = i_snprintf (out, avail, "enum { ");
  if (n < 0)
    {
      return n;
    }
  len += n;
  if (out)
    {
      out += n;
      if ((u32)n < avail)
        {
          avail -= n;
        }
      else
        {
          avail = 0;
        }
    }

  for (u16 i = 0; i < st->len; ++i)
    {
      string key = st->keys[i];

      n = snprintf (out, avail, "%.*s", key.len, key.data);
      if (n < 0)
        {
          return n;
        }
      len += n;
      if (out)
        {
          out += n;
          if ((u32)n < avail)
            {
              avail -= n;
            }
          else
            {
              avail = 0;
            }
        }

      if ((u16)(i + 1) < st->len)
        {
          n = snprintf (out, avail, ", ");
          if (n < 0)
            {
              return n;
            }
          len += n;
          if (out)
            {
              out += n;
              if ((u32)n < avail)
                {
                  avail -= n;
                }
              else
                {
                  avail = 0;
                }
            }
        }
    }

  n = snprintf (out, avail, " }");
  if (n < 0)
    {
      return n;
    }
  len += n;

  return len;
}

u32
enum_t_byte_size (const enum_t *t)
{
  valid_enum_t_assert (t);

  /**
   * Stored as a single byte
   */
  return sizeof (u8);
}

void
enum_t_free_internals_forgiving (enum_t *t, lalloc *alloc)
{
  if (!t)
    {
      return;
    }

  if (t->keys)
    {
      for (u32 i = 0; i < t->len; ++i)
        {
          if (t->keys[i].data)
            {
              lfree (alloc, t->keys[i].data);
            }

          t->keys[i].len = 0;
          t->keys[i].data = NULL;
        }
      lfree (alloc, t->keys);
      t->keys = NULL;
    }

  t->len = 0;
}

void
enum_t_free_internals (enum_t *t, lalloc *alloc)
{
  valid_enum_t_assert (t);

  for (u32 i = 0; i < t->len; ++i)
    {
      lfree (alloc, t->keys[i].data);
      t->keys[i].len = 0;
      t->keys[i].data = NULL;
    }
  lfree (alloc, t->keys);
  t->keys = NULL;
  t->len = 0;
}

u32
enum_t_get_serial_size (const enum_t *t)
{
  valid_enum_t_assert (t);
  u32 ret = 0;

  /**
   * LEN (DLEN DATA) (DLEN DATA) (DLEN DATA) ...
   */
  ret += sizeof (u16);
  ret += t->len * sizeof (u16);

  for (u32 i = 0; i < t->len; ++i)
    {
      ret += t->keys[0].len;
    }

  return ret;
}

TEST (enum_t_get_serial_size)
{
  enum_t en;
  en.len = 4;
  en.keys = (string[]){
    {
        .len = 3,
        .data = "foo",
    },
    {
        .len = 2,
        .data = "fo",
    },
    {
        .len = 4,
        .data = "baro",
    },
    {
        .len = 5,
        .data = "bazbi",
    },
  };

  // Expected
  u16 exp = 0;
  exp += sizeof (u16);                     // Len
  exp += 4 * sizeof (u16) + 3 + 2 + 4 + 5; // String keys

  u32 act = enum_t_byte_size (&en);

  test_assert_int_equal (exp, act);
}

void
enum_t_serialize (serializer *dest, const enum_t *src)
{
  valid_enum_t_assert (src);
  bool ret;

  /**
   * LEN (DLEN DATA) (DLEN DATA) (DLEN DATA) ...
   */
  ret = srlizr_write_u16 (dest, src->len);
  ASSERT (ret);

  for (u32 i = 0; i < src->len; ++i)
    {
      string next = src->keys[i];
      ret = srlizr_write_u16 (dest, next.len);
      ASSERT (ret);

      ret = srlizr_write (dest, (u8 *)next.data, next.len);
      ASSERT (ret);
    }
}

err_t
enum_t_deserialize (enum_t *dest, deserializer *src, lalloc *a)
{
  ASSERT (dest);

  err_t ret = SUCCESS;
  enum_t en = { 0 };

  /**
   * LEN
   */
  u16 len = 0;
  if (!dsrlizr_read_u16 (&len, src))
    {
      ret = ERR_INVALID_STATE;
      goto failed;
    }

  en.len = len;
  en.keys = lmalloc (a, len * sizeof *en.keys);

  if (en.keys == NULL)
    {
      ret = ERR_NOMEM;
      goto failed;
    }

  for (u32 i = 0; i < len; ++i)
    {
      /**
       * (KLEN
       */
      if (!dsrlizr_read_u16 (&len, src))
        {
          ret = ERR_INVALID_STATE;
          goto failed;
        }

      en.keys[i].data = lmalloc (a, len);
      if (en.keys[i].data == NULL)
        {
          ret = ERR_NOMEM;
          goto failed;
        }
      en.keys[i].len = len;

      /**
       * KEY)
       */
      if (!dsrlizr_read ((u8 *)en.keys[i].data, len, src))
        {
          ret = ERR_INVALID_STATE;
          goto failed;
        }
    }

  unchecked_enum_t_assert (&en);
  if (!enum_t_is_valid (&en))
    {
      ret = ERR_INVALID_STATE;
      goto failed;
    }
  valid_enum_t_assert (&en);

  *dest = en;
  return SUCCESS;

failed:
  enum_t_free_internals_forgiving (&en, a);
  return ret;
}
