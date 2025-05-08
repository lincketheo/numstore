#include "type/types/struct.h"
#include "dev/assert.h"
#include "dev/errors.h"
#include "dev/testing.h"
#include "ds/strings.h"
#include "intf/stdlib.h"
#include "intf/types.h"
#include "type/types.h"
#include "utils/deserializer.h"
#include "utils/serializer.h"

DEFINE_DBG_ASSERT_I (struct_t, unchecked_struct_t, s)
{
  ASSERT (s);
  ASSERT (s->keys);
  ASSERT (s->types);
}

static bool
struct_t_is_valid_shallow (const struct_t *s)
{
  unchecked_struct_t_assert (s);

  if (s->len == 0)
    {
      return false;
    }

  for (u32 i = 0; i < s->len; ++i)
    {
      if (s->keys[i].len == 0)
        {
          return false;
        }
      ASSERT (s->keys[i].data);
    }

  return strings_all_unique (s->keys, s->len);
}

DEFINE_DBG_ASSERT_I (struct_t, valid_struct_t, s)
{
  ASSERT (struct_t_is_valid_shallow (s));
}

bool
struct_t_is_valid (const struct_t *s)
{
  if (!struct_t_is_valid_shallow (s))
    {
      return false;
    }
  for (u32 i = 0; i < s->len; ++i)
    {
      if (!type_is_valid (&s->types[i]))
        {
          return false;
        }
    }
  return true;
}

int
struct_t_snprintf (char *str, u32 size, const struct_t *st)
{
  valid_struct_t_assert (st);

  char *out = str;
  u32 avail = size;
  int len = 0;
  int n;

  n = snprintf (out, avail, "struct { ");
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

  for (u32 i = 0; i < st->len; ++i)
    {
      string key = st->keys[i];
      n = snprintf (out, avail, "%.*s ", key.len, key.data);
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

      n = type_snprintf (out, avail, &st->types[i]);
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

      if (i + 1 < st->len)
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
struct_t_byte_size (const struct_t *t)
{
  valid_struct_t_assert (t);
  u32 ret = 0;

  /**
   * Each type is layed out contiguously
   */
  for (u32 i = 0; i < t->len; ++i)
    {
      ret += type_byte_size (&t->types[i]);
    }

  return ret;
}

TEST (struct_t_byte_size)
{
  struct_t st;
  st.len = 4;
  st.keys = (string[]){
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
  st.types = (type[]){
    {
        .type = T_PRIM,
        .p = U32,
    },
    {
        .type = T_PRIM,
        .p = U8,
    },
    {
        .type = T_PRIM,
        .p = U16,
    },
    {
        .type = T_PRIM,
        .p = CF128,
    },
  };

  u64 act = struct_t_byte_size (&st);
  u64 exp = (sizeof (u32) + sizeof (u8) + sizeof (u16) + sizeof (cf128));

  test_assert_int_equal (exp, act);
}

void
struct_t_free_internals_forgiving (struct_t *t, lalloc *alloc)
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

  if (t->types)
    {
      for (u32 i = 0; i < t->len; ++i)
        {
          type_free_internals_forgiving (&t->types[i], alloc);
          t->types[i] = (type){ 0 };
        }
      lfree (alloc, t->types);
      t->types = NULL;
    }

  t->len = 0;
}

void
struct_t_free_internals (struct_t *t, lalloc *alloc)
{
  valid_struct_t_assert (t);

  for (u32 i = 0; i < t->len; ++i)
    {
      lfree (alloc, t->keys[i].data);
      t->keys[i].len = 0;
      t->keys[i].data = NULL;

      type_free_internals (&t->types[i], alloc);
      t->types[i] = (type){ 0 };
    }

  lfree (alloc, t->keys);
  t->keys = NULL;

  lfree (alloc, t->types);
  t->types = NULL;

  t->len = 0;
}

u32
struct_t_get_serial_size (const struct_t *t)
{
  valid_struct_t_assert (t);
  u32 ret = 0;

  /**
   * LEN (KLEN KEY) (TYPE) (KLEN KEY) (TYPE) ....
   */
  ret += sizeof (u16);
  ret += t->len * sizeof (u16);

  for (u32 i = 0; i < t->len; ++i)
    {
      ret += t->keys[0].len;
      ret += type_get_serial_size (&t->types[i]);
    }

  return ret;
}

void
struct_t_serialize (serializer *dest, const struct_t *src)
{
  valid_struct_t_assert (src);
  bool ret;

  /**
   * LEN (KLEN KEY) (TYPE) (KLEN KEY) (TYPE) ....
   */
  ret = srlizr_write_u16 (dest, src->len);
  ASSERT (ret);

  for (u32 i = 0; i < src->len; ++i)
    {
      /**
       * (KLEN
       */
      string next = src->keys[i];
      ret = srlizr_write_u16 (dest, next.len);
      ASSERT (ret);

      /**
       * KEY)
       */
      ret = srlizr_write (dest, (u8 *)next.data, next.len);
      ASSERT (ret);

      /**
       * (TYPE)
       */
      type_serialize (dest, &src->types[i]);
    }
}

err_t
struct_t_deserialize (struct_t *dest, deserializer *src, lalloc *a)
{
  ASSERT (dest);

  err_t ret = SUCCESS;
  struct_t st = { 0 };

  /**
   * LEN
   */
  u16 len = 0;
  if (!dsrlizr_read_u16 (&len, src))
    {
      ret = ERR_INVALID_STATE;
      goto failed;
    }

  st.len = len;
  st.keys = lmalloc (a, len * sizeof *st.keys);
  st.types = lmalloc (a, len * sizeof *st.types);

  if (st.keys == NULL || st.types == NULL)
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

      st.keys[i].data = lmalloc (a, len);
      if (st.keys[i].data == NULL)
        {
          ret = ERR_NOMEM;
          goto failed;
        }
      st.keys[i].len = len;

      /**
       * KEY)
       */
      if (!dsrlizr_read ((u8 *)st.keys[i].data, len, src))
        {
          ret = ERR_INVALID_STATE;
          goto failed;
        }

      /**
       * (TYPE)
       */
      if ((ret = type_deserialize (&st.types[i], src, a)))
        {
          goto failed;
        }
    }

  unchecked_struct_t_assert (&st);
  if (!struct_t_is_valid_shallow (&st))
    {
      ret = ERR_INVALID_STATE;
      goto failed;
    }
  valid_struct_t_assert (&st);

  *dest = st;
  return SUCCESS;

failed:
  struct_t_free_internals_forgiving (&st, a);
  return ret;
}
