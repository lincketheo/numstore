#include "type/types/struct.h"
#include "dev/testing.h"
#include "ds/strings.h"
#include "errors/error.h"
#include "intf/stdlib.h"
#include "type/types.h"

DEFINE_DBG_ASSERT_I (struct_t, unchecked_struct_t, s)
{
  ASSERT (s);
  ASSERT (s->keys);
  ASSERT (s->types);
}

static err_t
struct_t_validate_shallow (const struct_t *s, error *e)
{
  unchecked_struct_t_assert (s);

  if (s->len == 0)
    {
      return error_causef (e, 1, "Struct key length must be > 0");
    }

  for (u32 i = 0; i < s->len; ++i)
    {
      if (s->keys[i].len == 0)
        {
          return error_causef (
              e, ERR_INVALID_TYPE,
              "Struct: length of key at index: %d is 0", i);
        }
      ASSERT (s->keys[i].data);
    }

  if (!strings_all_unique (s->keys, s->len))
    {
      return error_causef (
          e, ERR_INVALID_TYPE,
          "Struct: "
          "contains duplicate keys");
    }

  return SUCCESS;
}

DEFINE_DBG_ASSERT_I (struct_t, valid_struct_t, s)
{
  error e = error_create (NULL);
  ASSERT (struct_t_validate_shallow (s, &e) == SUCCESS);
}

err_t
struct_t_validate (const struct_t *s, error *e)
{
  err_t_wrap (struct_t_validate_shallow (s, e), e);
  {
    return false;
  }
  for (u32 i = 0; i < s->len; ++i)
    {
      err_t_wrap (type_validate (&s->types[i], e), e);
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
  /**
   * Program correctness: You have enough room in the serializer
   * to serialize an enum before serializing it
   */
  valid_struct_t_assert (src);
  bool ret;

  /**
   * LEN (KLEN KEY) (TYPE) (KLEN KEY) (TYPE) ....
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

      type_serialize (dest, &src->types[i]);
    }
}

TEST (enum_t_serialize)
{
  struct_t st;
  st.len = 5;
  st.keys = (string[]){
    { .len = },
    {

    },
    {

    },
    {

    }
  };
};
u8 act[200]; // Sloppy sizing
u8 exp[] = { 0, 0,
             0, 0, 'f', 'o', 'o',
             0, 0, 'f', 'o',
             0, 0, 'b', 'a', 'r', 'o',
             0, 0, 'b', 'a', 'z', 'b', 'i' };
u16 len = 4;
u16 l0 = 3;
u16 l2 = 2;
u16 l3 = 4;
u16 l4 = 5;
i_memcpy (&exp[0], &len, sizeof (u16));
i_memcpy (&exp[2], &l0, sizeof (u16));
i_memcpy (&exp[7], &l2, sizeof (u16));
i_memcpy (&exp[11], &l3, sizeof (u16));
i_memcpy (&exp[17], &l4, sizeof (u16));

// Expected
serializer s = srlizr_create (act, 200);
enum_t_serialize (&s, &en);

test_assert_int_equal (s.dlen, sizeof (exp));
test_assert_int_equal (i_memcmp (act, exp, sizeof (exp)), 0);
}

err_t
struct_t_deserialize (
    struct_t *dest,
    deserializer *src,
    lalloc *a,
    error *e)
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
      ret = error_causef (
          e, ERR_TYPE_DESER,
          "Struct Deserialize. Expected a length header");
      goto failed;
    }
  st.len = len;

  /**
   * Allocate Keys buffer
   */
  lalloc_r keys = lcalloc (a, len, len, sizeof *st.keys);
  if (keys.stat != AR_SUCCESS)
    {
      ret = error_causef (
          e, ERR_NOMEM,
          "Struct Deserialize: not enough "
          "memory to allocate keys buffer for %d keys",
          len);
      goto failed;
    }
  st.keys = keys.ret;

  /**
   * Allocate Types buffer
   */
  lalloc_r types = lcalloc (a, len, len, sizeof *st.types);
  if (keys.stat != AR_SUCCESS)
    {
      ret = error_causef (
          e, ERR_NOMEM,
          "Struct Deserialize: not enough "
          "memory to allocate type buffer for %d types",
          len);
      goto failed;
    }
  st.types = types.ret;

  for (u32 i = 0; i < len; ++i)
    {
      /**
       * (KLEN
       */
      if (!dsrlizr_read_u16 (&len, src))
        {
          ret = error_causef (
              e, ERR_TYPE_DESER,
              "Struct Deserialize. Expected a key length "
              "header for key: %d",
              i);
          goto failed;
        }

      lalloc_r data = lmalloc (a, len, len, 1);

      if (keys.stat != AR_SUCCESS)
        {
          ret = error_causef (
              e, ERR_NOMEM,
              "Struct Deserialize: not enough "
              "memory to allocate key: %d of length: %d",
              i, len);
          goto failed;
        }

      st.keys[i].data = data.ret;
      st.keys[i].len = len;

      /**
       * KEY)
       */
      if (!dsrlizr_read ((u8 *)st.keys[i].data, len, src))
        {
          ret = error_causef (
              e, ERR_TYPE_DESER,
              "Struct Deserialize. Expected %d bytes for key %d",
              len, i);
          goto failed;
        }

      /**
       * (TYPE)
       */
      if (type_deserialize (&st.types[i], src, a, e))
        {
          goto failed;
        }
    }

  unchecked_struct_t_assert (&st);
  err_t_wrap (struct_t_validate_shallow (&st, e), e);
  valid_struct_t_assert (&st);

  *dest = st;
  return SUCCESS;

failed:
  struct_t_free_internals_forgiving (&st, a);
  return ret;
}
