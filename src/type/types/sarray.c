#include "type/types/sarray.h"
#include "ds/strings.h"
#include "errors/error.h"
#include "intf/stdlib.h"
#include "type/types.h"

DEFINE_DBG_ASSERT_I (sarray_t, unchecked_sarray_t, s)
{
  ASSERT (s);
  ASSERT (s->dims);
  ASSERT (s->t);
  error e = error_create (NULL);
  ASSERT (sarray_t_validate (s, &e) == SUCCESS);
}

err_t
sarray_t_validate_shallow (const sarray_t *t, error *e)
{
  unchecked_sarray_t_assert (t);
  if (t->rank == 0)
    {
      return error_causef (e, 1, "Strict Array rank must be > 0");
    }
  for (u32 i = 0; i < t->rank; ++i)
    {
      if (t->dims[i] == 0)
        {
          return error_causef (
              e, ERR_INVALID_TYPE,
              "Strict Array: dim at %d is 0.", i);
        }
    }
  return SUCCESS;
}

DEFINE_DBG_ASSERT_I (sarray_t, valid_sarray_t, s)
{
  error e = error_create (NULL);
  ASSERT (sarray_t_validate_shallow (s, &e) == SUCCESS);
}

err_t
sarray_t_validate (const sarray_t *t, error *e)
{
  unchecked_sarray_t_assert (t);

  err_t_wrap (sarray_t_validate_shallow (t, e), e);
  err_t_wrap (type_validate (t->t, e), e);

  return SUCCESS;
}

int
sarray_t_snprintf (char *str, u32 size, const sarray_t *p)
{
  valid_sarray_t_assert (p);

  char *out = str;
  u32 avail = size;
  int len = 0;
  int n;

  for (u16 i = 0; i < p->rank; ++i)
    {
      n = i_snprintf (out, avail, "[%u]", p->dims[i]);
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

  n = i_snprintf (out, avail, " ");
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

  n = type_snprintf (out, avail, p->t);
  if (n < 0)
    {
      return n;
    }
  len += n;

  return len;
}
u32
sarray_t_byte_size (const sarray_t *t)
{
  valid_sarray_t_assert (t);
  u32 ret = 0;

  /**
   * multiply up all ranks and multiply by size of type
   */
  for (u32 i = 0; i < t->rank; ++i)
    {
      ret *= t->dims[i];
    }

  return ret * type_byte_size (t->t);
}

void
sarray_t_free_internals_forgiving (sarray_t *t, lalloc *alloc)
{
  if (!t)
    {
      return;
    }

  if (t->dims)
    {
      lfree (alloc, t->dims);
      t->dims = NULL;
    }
  if (t->t)
    {
      type_free_internals_forgiving (t->t, alloc);
      lfree (alloc, t->t);
      t->t = NULL;
    }
  t->rank = 0;
}

void
sarray_t_free_internals (sarray_t *t, lalloc *alloc)
{
  valid_sarray_t_assert (t);
  lfree (alloc, t->dims);
  type_free_internals (t->t, alloc);
  lfree (alloc, t->t);
  t->dims = NULL;
  t->rank = 0;
}

u32
sarray_t_get_serial_size (const sarray_t *t)
{
  valid_sarray_t_assert (t);
  u32 ret = 0;

  /**
   * RANK DIM0 DIM1 DIM2 ... TYPE
   */
  ret += sizeof (u16);
  ret += sizeof (u32) * t->rank;
  ret += type_get_serial_size (t->t);

  return ret;
}

void
sarray_t_serialize (serializer *dest, const sarray_t *src)
{
  valid_sarray_t_assert (src);
  bool ret;

  /**
   * RANK DIM0 DIM1 DIM2 ... TYPE
   */
  ret = srlizr_write_u16 (dest, src->rank);
  ASSERT (ret);

  for (u32 i = 0; i < src->rank; ++i)
    {
      /**
       * DIMi
       */
      ret = srlizr_write_u32 (dest, src->dims[i]);
      ASSERT (ret);
    }

  /**
   * (TYPE)
   */
  type_serialize (dest, src->t);
}

err_t
sarray_t_deserialize (sarray_t *dest, deserializer *src, lalloc *a, error *e)
{
  ASSERT (dest);

  err_t ret = SUCCESS;
  sarray_t sa = { 0 };

  /**
   * RANK
   */
  u16 rank = 0;
  if (!dsrlizr_read_u16 (&rank, src))
    {
      ret = error_causef (
          e, ERR_TYPE_DESER,
          "Strict Array Deserialize. Expected a rank header");
      goto failed;
    }
  sa.rank = rank;

  /**
   * Allocate dimensions buffer
   */
  lalloc_r dims = lcalloc (a, rank, rank, sizeof *sa.dims);
  if (dims.stat != AR_SUCCESS)
    {
      ret = error_causef (
          e, ERR_NOMEM,
          "Strict Array Deserialize: not enough "
          "memory to allocate dimensions buffer for rank: %d",
          rank);
      goto failed;
    }
  sa.dims = dims.ret;

  /**
   * Allocate type
   */
  lalloc_r t = lcalloc (a, 1, 1, sizeof *sa.t);
  if (dims.stat != AR_SUCCESS)
    {
      ret = error_causef (
          e, ERR_NOMEM,
          "Strict Array Deserialize: not enough "
          "memory to allocate type");
      goto failed;
    }
  sa.t = t.ret;

  if (sa.dims == NULL || sa.t == NULL)
    {
      ret = ERR_NOMEM;
      goto failed;
    }

  for (u32 i = 0; i < rank; ++i)
    {
      u32 dim;

      /**
       * DIMi
       */
      if (!dsrlizr_read_u32 (&dim, src))
        {
          ret = error_causef (
              e, ERR_TYPE_DESER,
              "Strict Array Deserialize. Expected dimension at index %d",
              i);
          goto failed;
        }

      sa.dims[i] = dim;
    }

  /**
   * (TYPE)
   */
  if ((type_deserialize (sa.t, src, a, e), e))
    {
      goto failed;
    }

  unchecked_sarray_t_assert (&sa);
  err_t_wrap (sarray_t_validate_shallow (&sa, e), e);
  valid_sarray_t_assert (&sa);

  *dest = sa;
  return SUCCESS;

failed:
  sarray_t_free_internals_forgiving (&sa, a);
  return ret;
}
