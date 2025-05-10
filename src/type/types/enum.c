#include "type/types/enum.h"
#include "dev/testing.h"
#include "intf/stdlib.h"

DEFINE_DBG_ASSERT_I (enum_t, unchecked_enum_t, e)
{
  ASSERT (e);
  ASSERT (e->keys);
}

DEFINE_DBG_ASSERT_I (enum_t, valid_enum_t, e)
{
  ASSERT (enum_t_validate (e, NULL) == SUCCESS);
}

err_t
enum_t_validate (const enum_t *en, error *e)
{
  unchecked_enum_t_assert (en);

  if (en->len == 0)
    {
      return error_causef (e, 1, "Enum key length must be > 0");
    }
  for (u32 i = 0; i < en->len; ++i)
    {
      if (en->keys[i].len == 0)
        {
          return error_causef (
              e, ERR_INVALID_TYPE,
              "Enum: length of key at index: %d is 0", i);
        }
      ASSERT (en->keys[i].data);
    }

  u32 i, j;
  if (!strings_all_unique_with_return (&i, &j, en->keys, en->len))
    {
      return error_causef (
          e, ERR_INVALID_TYPE,
          "Enum: Keys %d and %d are duplicates", i, j);
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
    .len = 5,
  };

  char buffer[200];
  const char *expected = "enum { FOO, BARZBI }";
  u32 len = i_unsafe_strlen (expected);

  int i = enum_t_snprintf (buffer, 200, &e);

  test_assert_int_equal (i, len);
  test_assert_int_equal (i_strncmp (expected, buffer, len), 0);
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

TEST (enum_t_free_internals_forgiving)
{
  lalloc temp = lalloc_create (1000);
  string *strings = lcalloc_test (&temp, 10, sizeof *strings);
  enum_t e = {
    .keys = strings,
    .len = 10,
  };
  enum_t_free_internals_forgiving (&e, &temp);
  test_assert_int_equal (temp.used, 0);
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

TEST (enum_t_free_internals)
{
  lalloc temp = lalloc_create (1000);
  string *strings = lcalloc_test (&temp, 10, sizeof *strings);
  for (u32 i = 0; i < 10; ++i)
    {
      strings[i].data = lcalloc_test (&temp, i + 4, 1);
      strings[i].len = i + 4;
    }
  enum_t e = {
    .keys = strings,
    .len = 10,
  };
  enum_t_free_internals (&e, &temp);
  test_assert_int_equal (temp.used, 0);
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
  u8 exp[] = { 4,
               3, 'f', 'o', 'o',
               2, 'f', 'o',
               4, 'b', 'a', 'r', 'o',
               5, 'b', 'a', 'z', 'b', 'i' };

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

  err_t ret = SUCCESS;
  enum_t en = { 0 };

  /**
   * LEN
   */
  u16 len = 0;
  if (!dsrlizr_read_u16 (&len, src))
    {
      ret = error_causef (
          e, ERR_TYPE_DESER,
          "Enum Deserialize. Expected a length header");
      goto failed;
    }

  lalloc_r keys = lcalloc (a, len, len, sizeof *en.keys);

  if (keys.stat != AR_SUCCESS)
    {
      ret = error_causef (
          e, ERR_NOMEM,
          "Enum Deserialize: not enough "
          "memory to allocate keys buffer for %d keys",
          len);
      goto failed;
    }

  en.keys = keys.ret;
  en.len = len;

  for (u32 i = 0; i < len; ++i)
    {
      /**
       * (KLEN
       */
      if (!dsrlizr_read_u16 (&len, src))
        {
          ret = error_causef (
              e, ERR_TYPE_DESER,
              "Enum Deserialize. Expected a key length "
              "header for key: %d",
              i);
          goto failed;
        }

      lalloc_r data = lmalloc (a, len, len, 1);

      if (keys.stat != AR_SUCCESS)
        {
          ret = error_causef (
              e, ERR_NOMEM,
              "Enum Deserialize: not enough "
              "memory to allocate key: %d of length: %d",
              i, len);
          goto failed;
        }

      en.keys[i].data = data.ret;
      en.keys[i].len = len;

      /**
       * KEY)
       */
      if (!dsrlizr_read ((u8 *)en.keys[i].data, len, src))
        {
          ret = error_causef (
              e, ERR_TYPE_DESER,
              "Enum Deserialize. Expected %d bytes for key %d",
              len, i);
          goto failed;
        }
    }

  unchecked_enum_t_assert (&en);

  err_t_wrap (enum_t_validate (&en, e), e);

  valid_enum_t_assert (&en);

  *dest = en;
  return SUCCESS;

failed:
  enum_t_free_internals_forgiving (&en, a);
  return ret;
}

TEST (enum_t_deserialize_green_path)
{
  u8 data[] = { 4,
                3, 'f', 'o', 'o',
                2, 'f', 'o',
                4, 'b', 'a', 'r', 'o',
                5, 'b', 'a', 'z', 'b', 'i' };

  lalloc en_alloc = lalloc_create (200);  // sloppy sizing
  lalloc er_alloc = lalloc_create (2000); // sloppy sizing

  deserializer d = dsrlizr_create (data, sizeof (data));

  error e = error_create (&er_alloc);

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

  enum_t_free_internals (&eret, &en_alloc);
  test_assert_int_equal (er_alloc.used, 0);
  test_assert_int_equal (en_alloc.used, 0);
}

TEST (enum_t_deserialize_red_path)
{
  u8 data[] = { 4,
                3, 'f', 'o', 'o',
                3, 'f', 'o', 'o',
                4, 'b', 'a', 'r', 'o',
                5, 'b', 'a', 'z', 'b', 'i' };

  lalloc en_alloc = lalloc_create (200);  // sloppy sizing
  lalloc er_alloc = lalloc_create (2000); // sloppy sizing

  deserializer d = dsrlizr_create (data, sizeof (data));

  error e = error_create (&er_alloc);

  enum_t eret;
  err_t ret = enum_t_deserialize (&eret, &d, &en_alloc, &e);

  test_assert_int_equal (ret, ERR_TYPE_DESER);
  test_assert_int_equal (
      string_equal (
          e.cause_msg,
          unsafe_cstrfrom ("Enum: Keys 0 and 1 are duplicates")),
      true);

  test_assert_int_equal (en_alloc.used, 0); // Cleans up on error

  error_reset (&e);
  test_assert_int_equal (er_alloc.used, 0);
}
