#include "numstore/type/types/union.h"

#include "core/dev/assert.h"  // DEFINE_DBG_ASSERT_I
#include "core/dev/testing.h" // TEST
#include "core/intf/stdlib.h" // i_strncmp
#include "core/math/random.h" // TODO

#include "numstore/type/builders/kvt.h" // kvt_builder

DEFINE_DBG_ASSERT_I (union_t, unchecked_union_t, s)
{
  ASSERT (s);
  ASSERT (s->keys);
  ASSERT (s->types);
}

static inline err_t
union_t_nomem (const char *msg, error *e)
{
  return error_causef (e, ERR_NOMEM, "Union: %s", msg);
}

static inline err_t
union_t_type_err (const char *msg, error *e)
{
  return error_causef (e, ERR_INVALID_ARGUMENT, "Union: %s", msg);
}

static inline err_t
union_t_type_deser (const char *msg, error *e)
{
  return error_causef (e, ERR_TYPE_DESER, "Union: %s", msg);
}

static err_t
union_t_validate_shallow (const union_t *s, error *e)
{
  unchecked_union_t_assert (s);

  if (s->len == 0)
    {
      return union_t_type_err ("Keys length must be > 0", e);
    }

  for (u32 i = 0; i < s->len; ++i)
    {
      if (s->keys[i].len == 0)
        {
          return union_t_type_err ("Key length must be > 0", e);
        }
      ASSERT (s->keys[i].data);
    }

  if (!strings_all_unique (s->keys, s->len))
    {
      return union_t_type_err ("Duplicate keys", e);
    }

  return SUCCESS;
}

DEFINE_DBG_ASSERT_I (union_t, valid_union_t, s)
{
  error e = error_create (NULL);
  ASSERT (union_t_validate_shallow (s, &e) == SUCCESS);
}

err_t
union_t_validate (const union_t *s, error *e)
{
  err_t_wrap (union_t_validate_shallow (s, e), e);
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

i32
union_t_snprintf (char *str, u32 size, const union_t *st)
{
  valid_union_t_assert (st);

  char *out = str;
  u32 avail = size;
  int len = 0;
  int n;

  n = snprintf (out, avail, "union { ");
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

TEST (union_t_snprintf)
{
  union_t st;
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

  char buffer[200];
  const char *expected = "union { foo U32, fo U8, baro U16, bazbi CF128 }";
  u32 len = i_unsafe_strlen (expected);

  int i = union_t_snprintf (buffer, 200, &st);

  test_assert_int_equal (i, len);
  test_assert_int_equal (i_strncmp (expected, buffer, len), 0);
}

u32
union_t_byte_size (const union_t *t)
{
  valid_union_t_assert (t);
  u32 ret = 0;

  for (u32 i = 0; i < t->len; ++i)
    {
      u32 next = type_byte_size (&t->types[i]);
      if (next > ret)
        {
          ret = next;
        }
    }

  return ret;
}

TEST (union_t_byte_size)
{
  union_t st;
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

  u64 act = union_t_byte_size (&st);
  u64 exp = sizeof (cf128);

  test_assert_int_equal (exp, act);
}

u32
union_t_get_serial_size (const union_t *t)
{
  valid_union_t_assert (t);
  u32 ret = 0;

  /**
   * LEN (KLEN KEY) (TYPE) (KLEN KEY) (TYPE) ....
   */
  ret += sizeof (u16);
  ret += t->len * sizeof (u16);

  for (u32 i = 0; i < t->len; ++i)
    {
      ret += t->keys[i].len;
      ret += type_get_serial_size (&t->types[i]);
    }

  return ret;
}

TEST (union_t_get_serial_size)
{
  union_t st;
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

  u64 act = union_t_get_serial_size (&st);
  u64 exp = 2 + 4 * 2 + 3 + 2 + 4 + 5 + 4 * 2;

  test_assert_int_equal (exp, act);
}

void
union_t_serialize (serializer *dest, const union_t *src)
{
  valid_union_t_assert (src);
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

TEST (union_t_serialize)
{
  union_t st;
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

  u8 act[200]; // Sloppy sizing
  u8 exp[] = { 0, 0,
               0, 0, 'f', 'o', 'o', (u8)T_PRIM, (u8)U32,
               0, 0, 'f', 'o', (u8)T_PRIM, (u8)U8,
               0, 0, 'b', 'a', 'r', 'o', (u8)T_PRIM, (u8)U16,
               0, 0, 'b', 'a', 'z', 'b', 'i', (u8)T_PRIM, (u8)CF128 };
  u16 len = 4;
  u16 l0 = 3;
  u16 l2 = 2;
  u16 l3 = 4;
  u16 l4 = 5;
  i_memcpy (&exp[0], &len, sizeof (u16));
  i_memcpy (&exp[2], &l0, sizeof (u16));
  i_memcpy (&exp[9], &l2, sizeof (u16));
  i_memcpy (&exp[15], &l3, sizeof (u16));
  i_memcpy (&exp[23], &l4, sizeof (u16));

  // Expected
  serializer s = srlizr_create (act, 200);
  union_t_serialize (&s, &st);

  test_assert_int_equal (s.dlen, sizeof (exp));
  test_assert_int_equal (i_memcmp (act, exp, sizeof (exp)), 0);
}

err_t
union_t_deserialize (
    union_t *dest,
    deserializer *src,
    lalloc *a,
    error *e)
{
  ASSERT (dest);

  u8 working[2048];
  lalloc balloc = lalloc_create (working, sizeof (working));
  kvt_builder unb = kvb_create (&balloc, a);

  /**
   * LEN
   */
  u16 len;
  if (!dsrlizr_read_u16 (&len, src))
    {
      goto early_termination;
    }

  for (u32 i = 0; i < len; ++i)
    {
      string key;
      if (!dsrlizr_read_u16 (&key.len, src))
        {
          goto early_termination;
        }

      key.data = lmalloc (a, key.len, 1);
      if (key.data == NULL)
        {
          return union_t_nomem ("Allocating key", e);
        }

      if (!dsrlizr_read ((u8 *)key.data, key.len, src))
        {
          goto early_termination;
        }

      type t;
      err_t_wrap (type_deserialize (&t, src, a, e), e);

      err_t_wrap (kvb_accept_key (&unb, key, e), e);
      err_t_wrap (kvb_accept_type (&unb, t, e), e);
    }

  err_t_wrap (kvb_union_t_build (dest, &unb, e), e);

  return SUCCESS;

early_termination:
  return union_t_type_deser ("Early end of serialized string", e);
}

TEST (union_t_deserialize_green_path)
{
  u8 data[] = { 0, 0,
                0, 0, 'f', 'o', 'o', (u8)T_PRIM, (u8)U32,
                0, 0, 'f', 'o', (u8)T_PRIM, (u8)U8,
                0, 0, 'b', 'a', 'r', 'o', (u8)T_PRIM, (u8)U16,
                0, 0, 'b', 'a', 'z', 'b', 'i', (u8)T_PRIM, (u8)CF128 };
  u16 len = 4;
  u16 l0 = 3;
  u16 l2 = 2;
  u16 l3 = 4;
  u16 l4 = 5;
  i_memcpy (&data[0], &len, sizeof (u16));
  i_memcpy (&data[2], &l0, sizeof (u16));
  i_memcpy (&data[9], &l2, sizeof (u16));
  i_memcpy (&data[15], &l3, sizeof (u16));
  i_memcpy (&data[23], &l4, sizeof (u16));

  char space[2000];
  lalloc st_alloc = lalloc_create ((u8 *)space, sizeof (space));

  deserializer d = dsrlizr_create (data, sizeof (data));

  error e = error_create (NULL);

  union_t eret;
  err_t ret = union_t_deserialize (&eret, &d, &st_alloc, &e);

  test_assert_int_equal (ret, SUCCESS);

  test_assert_int_equal (eret.len, 4);

  test_assert_int_equal (eret.keys[0].len, 3);
  test_assert_int_equal (i_memcmp (eret.keys[0].data, "foo", 3), 0);
  test_assert_int_equal (eret.types[0].type, T_PRIM);
  test_assert_int_equal (eret.types[0].p, U32);

  test_assert_int_equal (eret.keys[1].len, 2);
  test_assert_int_equal (i_memcmp (eret.keys[1].data, "fo", 2), 0);
  test_assert_int_equal (eret.types[1].type, T_PRIM);
  test_assert_int_equal (eret.types[1].p, U8);

  test_assert_int_equal (eret.keys[2].len, 4);
  test_assert_int_equal (i_memcmp (eret.keys[2].data, "baro", 4), 0);
  test_assert_int_equal (eret.types[2].type, T_PRIM);
  test_assert_int_equal (eret.types[2].p, U16);

  test_assert_int_equal (eret.keys[3].len, 5);
  test_assert_int_equal (i_memcmp (eret.keys[3].data, "bazbi", 5), 0);
  test_assert_int_equal (eret.types[3].type, T_PRIM);
  test_assert_int_equal (eret.types[3].p, CF128);
}

TEST (union_t_deserialize_red_path)
{
  u8 data[] = { 0, 0,
                0, 0, 'f', 'o', 'o', (u8)T_PRIM, (u8)U32,
                0, 0, 'f', 'o', 'o', (u8)T_PRIM, (u8)U8,
                0, 0, 'b', 'a', 'r', 'o', (u8)T_PRIM, (u8)U16,
                0, 0, 'b', 'a', 'z', 'b', 'i', (u8)T_PRIM, (u8)CF128 };
  u16 len = 4;
  u16 l0 = 3;
  u16 l2 = 3;
  u16 l3 = 4;
  u16 l4 = 5;
  i_memcpy (&data[0], &len, sizeof (u16));
  i_memcpy (&data[2], &l0, sizeof (u16));
  i_memcpy (&data[9], &l2, sizeof (u16));
  i_memcpy (&data[16], &l3, sizeof (u16));
  i_memcpy (&data[24], &l4, sizeof (u16));

  union_t sret;
  char space[2000];
  lalloc alloc = lalloc_create ((u8 *)space, sizeof (space));
  deserializer d = dsrlizr_create (data, sizeof (data));

  error e = error_create (NULL);
  err_t ret = union_t_deserialize (&sret, &d, &alloc, &e);

  test_assert_int_equal (ret, ERR_INVALID_ARGUMENT); // Duplicate
}

err_t
union_t_random (union_t *un, lalloc *alloc, u32 depth, error *e)
{
  ASSERT (un);

  un->len = (u16)randu32 (1, 5);

  un->keys = (string *)lmalloc (alloc, un->len, sizeof (string));
  if (!un->keys)
    {
      return error_causef (e, ERR_NOMEM, "Union keys");
    }

  un->types = (type *)lmalloc (alloc, un->len, sizeof (type));
  if (!un->types)
    {
      return error_causef (e, ERR_NOMEM, "Union types");
    }

  for (u16 i = 0; i < un->len; ++i)
    {
      err_t_wrap (rand_str (&un->keys[i], alloc, 5, 11, e), e);
      err_t_wrap (type_random (&un->types[i], alloc, depth - 1, e), e);
    }

  return SUCCESS;
}

bool
union_t_equal (const union_t *left, const union_t *right)
{
  if (left->len != right->len)
    {
      return false;
    }

  for (u32 i = 0; i < left->len; ++i)
    {
      if (!string_equal (left->keys[i], right->keys[i]))
        {
          return false;
        }
      if (!type_equal (&left->types[i], &right->types[i]))
        {
          return false;
        }
    }

  return true;
}
