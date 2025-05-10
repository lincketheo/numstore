#include "type/types/union.h"
#include "intf/stdlib.h"
#include "type/types.h"

DEFINE_DBG_ASSERT_I (union_t, unchecked_union_t, s)
{
  ASSERT (s);
  ASSERT (s->keys);
  ASSERT (s->types);
}

static err_t
union_t_validate_shallow (const union_t *s, error *e)
{
  unchecked_union_t_assert (s);

  if (s->len == 0)
    {
      return error_causef (e, 1, "Union key length must be > 0");
    }

  for (u32 i = 0; i < s->len; ++i)
    {
      if (s->keys[i].len == 0)
        {
          return error_causef (
              e, ERR_INVALID_TYPE,
              "Union: length of key at index: %d is 0", i);
        }
      ASSERT (s->keys[i].data);
    }
  u32 i, j;
  if (!strings_all_unique_with_return (&i, &j, s->keys, s->len))
    {
      return error_causef (
          e, ERR_INVALID_TYPE,
          "Union: Keys %d and %d are duplicates", i, j);
    }

  return SUCCESS;
}

DEFINE_DBG_ASSERT_I (union_t, valid_union_t, s)
{
  ASSERT (union_t_validate_shallow (s, NULL) == SUCCESS);
}

err_t
union_t_validate (const union_t *t, error *e)
{
  err_t_wrap (union_t_validate_shallow (t, e), e);

  for (u32 i = 0; i < t->len; ++i)
    {
      err_t_wrap (type_validate (&t->types[i], e), e);
    }
  return SUCCESS;
}

int
union_t_snprintf (char *str, u32 size, const union_t *p)
{
  valid_union_t_assert (p);

  char *out = str;
  u32 avail = size;
  int len = 0;
  int n;

  n = i_snprintf (out, avail, "union { ");
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

  for (u16 i = 0; i < p->len; ++i)
    {
      string key = p->keys[i];
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

      n = snprintf (out, avail, " ");
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

      n = type_snprintf (out, avail, &p->types[i]);
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

      if ((u16)(i + 1) < p->len)
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

  n = i_snprintf (out, avail, " }");
  if (n < 0)
    {
      return n;
    }
  len += n;

  return len;
}

u32
union_t_byte_size (const union_t *t)
{
  valid_union_t_assert (t);
  u32 ret = 0;

  /**
   * Get the maximum size
   */
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

void
union_t_free_internals_forgiving (union_t *t, lalloc *alloc)
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
union_t_free_internals (union_t *t, lalloc *alloc)
{
  valid_union_t_assert (t);

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
      ret += t->keys[0].len;
      ret += type_get_serial_size (&t->types[i]);
    }

  return ret;
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

err_t
union_t_deserialize (
    union_t *dest,
    deserializer *src,
    lalloc *a,
    error *e)
{
  ASSERT (dest);

  err_t ret = SUCCESS;
  union_t un = { 0 };

  /**
   * LEN
   */
  u16 len = 0;
  if (!dsrlizr_read_u16 (&len, src))
    {
      ret = error_causef (
          e, ERR_TYPE_DESER,
          "Union Deserialize. Expected a length header");
      goto failed;
    }
  un.len = len;

  /**
   * Allocate Keys buffer
   */
  lalloc_r keys = lcalloc (a, len, len, sizeof *un.keys);
  if (keys.stat != AR_SUCCESS)
    {
      ret = error_causef (
          e, ERR_NOMEM,
          "Union Deserialize: not enough "
          "memory to allocate keys buffer for %d keys",
          len);
      goto failed;
    }
  un.keys = keys.ret;

  /**
   * Allocate Types buffer
   */
  lalloc_r types = lcalloc (a, len, len, sizeof *un.types);
  if (keys.stat != AR_SUCCESS)
    {
      ret = error_causef (
          e, ERR_NOMEM,
          "Union Deserialize: not enough "
          "memory to allocate type buffer for %d types",
          len);
      goto failed;
    }
  un.types = types.ret;

  for (u32 i = 0; i < len; ++i)
    {
      /**
       * (KLEN
       */
      if (!dsrlizr_read_u16 (&len, src))
        {
          ret = error_causef (
              e, ERR_TYPE_DESER,
              "Union Deserialize. Expected a key length "
              "header for key: %d",
              i);
          goto failed;
        }

      lalloc_r data = lmalloc (a, len, len, 1);

      if (keys.stat != AR_SUCCESS)
        {
          ret = error_causef (
              e, ERR_NOMEM,
              "Union Deserialize: not enough "
              "memory to allocate key: %d of length: %d",
              i, len);
          goto failed;
        }

      un.keys[i].data = data.ret;
      un.keys[i].len = len;

      /**
       * KEY)
       */
      if (!dsrlizr_read ((u8 *)un.keys[i].data, len, src))
        {
          ret = error_causef (
              e, ERR_TYPE_DESER,
              "Union Deserialize. Expected %d bytes for key %d",
              len, i);
          goto failed;
        }

      /**
       * (TYPE)
       */
      if (type_deserialize (&un.types[i], src, a, e))
        {
          goto failed;
        }
    }

  unchecked_union_t_assert (&un);
  err_t_wrap (union_t_validate_shallow (&un, e), e);
  valid_union_t_assert (&un);

  *dest = un;
  return SUCCESS;

failed:
  union_t_free_internals_forgiving (&un, a);
  return ret;
}
