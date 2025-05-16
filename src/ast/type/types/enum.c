#include "ast/type/types/enum.h"

#include "dev/assert.h"  // DEFINE_DBG_ASSERT_I
#include "dev/testing.h" // TEST
#include "intf/stdlib.h" // i_snprintf

DEFINE_DBG_ASSERT_I (enum_t, unchecked_enum_t, e)
{
  ASSERT (e);
  ASSERT (e->keys);
}

DEFINE_DBG_ASSERT_I (enum_t, valid_enum_t, e)
{
  error e_ = error_create (NULL);
  ASSERT (enum_t_validate (e, &e_) == SUCCESS);
}

static inline err_t
enum_t_nomem (const char *msg, error *e)
{
  return error_causef (e, ERR_NOMEM, "Enum: %s", msg);
}

static inline err_t
enum_t_type_err (const char *msg, error *e)
{
  return error_causef (e, ERR_INVALID_ARGUMENT, "Enum: %s", msg);
}

static inline err_t
enum_t_type_deser (const char *msg, error *e)
{
  return error_causef (e, ERR_TYPE_DESER, "Enum: %s", msg);
}

err_t
enum_t_validate (const enum_t *en, error *e)
{
  unchecked_enum_t_assert (en);

  if (en->len == 0)
    {
      return enum_t_type_err ("Number of keys must be > 0", e);
    }
  for (u32 i = 0; i < en->len; ++i)
    {
      if (en->keys[i].len == 0)
        {
          return enum_t_type_err ("Key length must be > 0", e);
        }
      ASSERT (en->keys[i].data);
    }

  if (!strings_all_unique (en->keys, en->len))
    {
      return enum_t_type_err ("Duplicate key", e);
    }

  return SUCCESS;
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

      n = i_snprintf (out, avail, "%.*s", key.len, key.data);
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
          n = i_snprintf (out, avail, ", ");
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

  n = i_snprintf (out, avail, " }");
  if (n < 0)
    {
      return n;
    }
  len += n;

  return len;
}

TEST (enum_t_snprintf)
{
  string keys[2] = {
    unsafe_cstrfrom ("FOO"),
    unsafe_cstrfrom ("BARZBI"),
  };

  enum_t e = {
    .keys = keys,
    .len = 2,
  };

  char buffer[200];
  const char *expected = "enum { FOO, BARZBI }";
  u32 len = i_unsafe_strlen (expected);

  int i = enum_t_snprintf (buffer, 200, &e);

  test_assert_int_equal (i, len);
  test_assert_int_equal (i_strncmp (expected, buffer, len), 0);
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
      ret += t->keys[i].len;
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

  u32 act = enum_t_get_serial_size (&en);

  test_assert_int_equal (exp, act);
}

void
enum_t_serialize (serializer *dest, const enum_t *src)
{
  /**
   * Program correctness: You have enough room in the serializer
   * to serialize an enum before serializing it
   */
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

TEST (enum_t_serialize)
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
enum_t_deserialize (enum_t *dest, deserializer *src, lalloc *a, error *e)
{
  ASSERT (dest);

  enum_t en = { 0 };

  /**
   * LEN
   */
  if (!dsrlizr_read_u16 (&en.len, src))
    {
      goto early_termination;
    }

  string *keys = lcalloc (a, en.len, sizeof *keys);
  if (keys == NULL)
    {
      return enum_t_nomem ("Allocating keys buffer", e);
    }
  en.keys = keys;

  for (u32 i = 0; i < en.len; ++i)
    {
      /**
       * (KLEN
       */
      if (!dsrlizr_read_u16 (&en.keys[i].len, src))
        {
          goto early_termination;
        }

      char *data = lmalloc (a, en.keys[i].len, 1);
      if (data == NULL)
        {
          return enum_t_nomem ("Allocating key", e);
        }

      en.keys[i].data = data;

      /**
       * KEY)
       */
      if (!dsrlizr_read ((u8 *)en.keys[i].data, en.keys[i].len, src))
        {
          goto early_termination;
        }
    }

  err_t_wrap (enum_t_validate (&en, e), e);

  *dest = en;
  return SUCCESS;

early_termination:
  return enum_t_type_deser ("Early end of serialized string", e);
}

TEST (enum_t_deserialize_green_path)
{
  u8 data[] = { 0, 0,
                0, 0, 'f', 'o', 'o',
                0, 0, 'f', 'o',
                0, 0, 'b', 'a', 'r', 'o',
                0, 0, 'b', 'a', 'z', 'b', 'i' };
  u16 len = 4;
  u16 l0 = 3;
  u16 l2 = 2;
  u16 l3 = 4;
  u16 l4 = 5;
  i_memcpy (&data[0], &len, sizeof (u16));
  i_memcpy (&data[2], &l0, sizeof (u16));
  i_memcpy (&data[7], &l2, sizeof (u16));
  i_memcpy (&data[11], &l3, sizeof (u16));
  i_memcpy (&data[17], &l4, sizeof (u16));

  char _backing[2000];
  lalloc en_alloc = lalloc_create ((u8 *)_backing, sizeof (_backing));

  deserializer d = dsrlizr_create (data, sizeof (data));

  error e = error_create (NULL);

  enum_t eret;
  err_t ret = enum_t_deserialize (&eret, &d, &en_alloc, &e);

  test_assert_int_equal (ret, SUCCESS);

  test_assert_int_equal (eret.len, 4);

  test_assert_int_equal (eret.keys[0].len, 3);
  test_assert_int_equal (i_memcmp (eret.keys[0].data, "foo", 3), 0);

  test_assert_int_equal (eret.keys[1].len, 2);
  test_assert_int_equal (i_memcmp (eret.keys[1].data, "fo", 2), 0);

  test_assert_int_equal (eret.keys[2].len, 4);
  test_assert_int_equal (i_memcmp (eret.keys[2].data, "baro", 4), 0);

  test_assert_int_equal (eret.keys[3].len, 5);
  test_assert_int_equal (i_memcmp (eret.keys[3].data, "bazbi", 5), 0);
}

TEST (enum_t_deserialize_red_path)
{
  u8 data[] = { 0, 0,
                0, 0, 'f', 'o', 'o', // Duplicate
                0, 0, 'f', 'o', 'o', // Duplicate
                0, 0, 'b', 'a', 'r', 'o',
                0, 0, 'b', 'a', 'z', 'b', 'i' };
  u16 len = 4;
  u16 l0 = 3;
  u16 l2 = 3;
  u16 l3 = 4;
  u16 l4 = 5;
  i_memcpy (&data[0], &len, sizeof (u16));
  i_memcpy (&data[2], &l0, sizeof (u16));
  i_memcpy (&data[7], &l2, sizeof (u16));
  i_memcpy (&data[12], &l3, sizeof (u16));
  i_memcpy (&data[18], &l4, sizeof (u16));

  enum_t eret;
  char _backing[2000];
  lalloc alloc = lalloc_create ((u8 *)_backing, sizeof (_backing));
  deserializer d = dsrlizr_create (data, sizeof (data));

  error e = error_create (NULL);
  err_t ret = enum_t_deserialize (&eret, &d, &alloc, &e);

  test_assert_int_equal (ret, ERR_INVALID_ARGUMENT); // Duplicate
}
