#include "numstore/type/types/sarray.h"

#include "core/dev/assert.h"  // DEFINE_DBG_ASSERT_I
#include "core/dev/testing.h" // TEST
#include "core/intf/stdlib.h" // i_snprintf
#include "core/math/random.h" // randu32

#include "numstore/type/types.h" // type_validate

DEFINE_DBG_ASSERT_I (sarray_t, unchecked_sarray_t, s)
{
  ASSERT (s);
  ASSERT (s->dims);
  ASSERT (s->t);
}

DEFINE_DBG_ASSERT_I (sarray_t, valid_sarray_t, s)
{
  error e = error_create (NULL);
  ASSERT (sarray_t_validate (s, &e) == SUCCESS);
}

static inline err_t
sarray_t_nomem (const char *msg, error *e)
{
  return error_causef (e, ERR_NOMEM, "Strict Array: %s", msg);
}

static inline err_t
sarray_t_type_err (const char *msg, error *e)
{
  return error_causef (e, ERR_INVALID_ARGUMENT, "Strict Array: %s", msg);
}

static inline err_t
sarray_t_type_deser (const char *msg, error *e)
{
  return error_causef (e, ERR_TYPE_DESER, "Strict Array: %s", msg);
}

err_t
sarray_t_validate_shallow (const sarray_t *t, error *e)
{
  unchecked_sarray_t_assert (t);
  if (t->rank == 0)
    {
      return sarray_t_type_err ("Rank must be > 0", e);
    }
  for (u32 i = 0; i < t->rank; ++i)
    {
      if (t->dims[i] == 0)
        {
          return sarray_t_type_err ("dimensions cannot be 0", e);
        }
    }
  return SUCCESS;
}

err_t
sarray_t_validate (const sarray_t *t, error *e)
{
  unchecked_sarray_t_assert (t);

  err_t_wrap (sarray_t_validate_shallow (t, e), e);
  err_t_wrap (type_validate (t->t, e), e);

  return SUCCESS;
}

i32
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

  n = type_snprintf (out, avail, p->t);
  if (n < 0)
    {
      return n;
    }
  len += n;

  return len;
}

TEST (sarray_t_snprintf)
{
  sarray_t s = {
    .dims = (u32[]){ 10, 11, 12 },
    .rank = 3,
    .t = &(type){
        .type = T_PRIM,
        .p = U32,
    },
  };

  char buffer[200];
  const char *expected = "[10][11][12]U32";
  u32 len = i_unsafe_strlen (expected);

  int i = sarray_t_snprintf (buffer, 200, &s);

  test_assert_int_equal (i, len);
  test_assert_int_equal (i_strncmp (expected, buffer, len), 0);
}

u32
sarray_t_byte_size (const sarray_t *t)
{
  valid_sarray_t_assert (t);
  u32 ret = 1;

  /**
   * multiply up all ranks and multiply by size of type
   */
  for (u32 i = 0; i < t->rank; ++i)
    {
      ret *= t->dims[i];
    }

  return ret * type_byte_size (t->t);
}

TEST (sarray_t_byte_size)
{
  sarray_t s = {
    .dims = (u32[]){ 10, 11, 12 },
    .rank = 3,
    .t = &(type){
        .type = T_PRIM,
        .p = U32,
    },
  };
  test_assert_int_equal (sarray_t_byte_size (&s), 10 * 11 * 12 * 4);
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

TEST (sarray_t_get_serial_size)
{
  sarray_t s = {
    .dims = (u32[]){ 10, 11, 12 },
    .rank = 3,
    .t = &(type){
        .type = T_PRIM,
        .p = U32,
    },
  };
  test_assert_int_equal (sarray_t_get_serial_size (&s), 3 * 4 + 2 + 2);
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

TEST (sarray_t_serialize)
{
  sarray_t s = {
    .dims = (u32[]){ 10, 11, 12 },
    .rank = 3,
    .t = &(type){
        .type = T_PRIM,
        .p = U32,
    },
  };

  u8 act[200];
  u8 exp[] = { 0, 0,
               0, 0, 0, 0,
               0, 0, 0, 0,
               0, 0, 0, 0,
               (u8)T_PRIM, (u8)U32 };
  u16 len = 3;
  u32 d0 = 10;
  u32 d1 = 11;
  u32 d2 = 12;
  i_memcpy (exp, &len, 2);
  i_memcpy (exp + 2, &d0, 4);
  i_memcpy (exp + 6, &d1, 4);
  i_memcpy (exp + 10, &d2, 4);

  serializer sr = srlizr_create (act, 200);
  sarray_t_serialize (&sr, &s);

  test_assert_int_equal (sr.dlen, sizeof (exp));
  test_assert_int_equal (i_memcmp (act, exp, sizeof (exp)), 0);
}

err_t
sarray_t_deserialize (sarray_t *dest, deserializer *src, lalloc *a, error *e)
{
  ASSERT (dest);

  sarray_t sa = { 0 };

  /**
   * RANK
   */
  if (!dsrlizr_read_u16 (&sa.rank, src))
    {
      goto early_terimination;
    }

  /**
   * Allocate dimensions buffer
   */
  u32 *dims = lmalloc (a, sa.rank, sizeof *dims);
  if (dims == NULL)
    {
      return sarray_t_nomem ("Allocating dims buffer", e);
    }
  sa.dims = dims;

  /**
   * Allocate type
   */
  type *t = lmalloc (a, 1, sizeof *t);
  if (t == NULL)
    {
      return sarray_t_nomem ("Allocating subtype", e);
    }
  sa.t = t;

  for (u32 i = 0; i < sa.rank; ++i)
    {
      u32 dim;

      /**
       * DIMi
       */
      if (!dsrlizr_read_u32 (&dim, src))
        {
          goto early_terimination;
        }

      sa.dims[i] = dim;
    }

  /**
   * (TYPE)
   */
  err_t_wrap (type_deserialize (sa.t, src, a, e), e);
  err_t_wrap (sarray_t_validate_shallow (&sa, e), e);

  *dest = sa;
  return SUCCESS;

early_terimination:
  return sarray_t_type_deser ("Early end of serialized string", e);
}

TEST (sarray_t_deserialize_green_path)
{
  u8 data[] = { 0, 0,
                0, 0, 0, 0,
                0, 0, 0, 0,
                0, 0, 0, 0,
                (u8)T_PRIM, (u8)U32 };
  u16 len = 3;
  u32 d0 = 10;
  u32 d1 = 11;
  u32 d2 = 12;
  i_memcpy (data, &len, 2);
  i_memcpy (data + 2, &d0, 4);
  i_memcpy (data + 6, &d1, 4);
  i_memcpy (data + 10, &d2, 4);

  u8 *_backing[2000];
  lalloc sab_alloc = lalloc_create ((u8 *)_backing, sizeof (_backing));

  deserializer d = dsrlizr_create (data, sizeof (data));

  error e = error_create (NULL);

  sarray_t sret;
  err_t ret = sarray_t_deserialize (&sret, &d, &sab_alloc, &e);

  test_assert_int_equal (ret, SUCCESS);

  test_assert_int_equal (sret.rank, 3);

  test_assert_int_equal (sret.dims[0], 10);
  test_assert_int_equal (sret.dims[1], 11);
  test_assert_int_equal (sret.dims[2], 12);
}

TEST (sarray_t_deserialize_red_path)
{
  u8 data[] = { 0, 0,
                0, 0, 0, 0,
                0, 0, 0, 0,
                0, 0, 0, 0,
                (u8)T_PRIM, (u8)U32 };
  u16 len = 3;
  u32 d0 = 10;
  u32 d1 = 0;
  u32 d2 = 12;
  i_memcpy (data, &len, 2);
  i_memcpy (data + 2, &d0, 4);
  i_memcpy (data + 6, &d1, 4);
  i_memcpy (data + 10, &d2, 4);

  sarray_t eret;
  u8 *_backing[2000];
  lalloc alloc = lalloc_create ((u8 *)_backing, sizeof (_backing));
  deserializer d = dsrlizr_create (data, sizeof (data));

  error e = error_create (NULL);
  err_t ret = sarray_t_deserialize (&eret, &d, &alloc, &e);

  test_assert_int_equal (ret, ERR_INVALID_ARGUMENT); // 0 value
}

err_t
sarray_t_random (sarray_t *sa, lalloc *alloc, u32 depth, error *e)
{
  ASSERT (sa);

  sa->rank = (u16)randu32 (1, 4);

  sa->dims = (u32 *)lmalloc (alloc, sa->rank, sizeof (u32));
  if (!sa->dims)
    {
      return error_causef (e, ERR_NOMEM, "Sarray dims");
    }

  for (u16 i = 0; i < sa->rank; ++i)
    {
      sa->dims[i] = randu32 (1, 11);
    }

  sa->t = (type *)lmalloc (alloc, 1, sizeof (type));
  if (!sa->t)
    {
      return error_causef (e, ERR_NOMEM, "Sarray sub-type");
    }

  err_t_wrap (type_random (sa->t, alloc, depth - 1, e), e);

  return SUCCESS;
}

bool
sarray_t_equal (const sarray_t *left, const sarray_t *right)
{
  if (left->rank != right->rank)
    {
      return false;
    }

  for (u32 i = 0; i < left->rank; ++i)
    {
      if (left->dims[i] != right->dims[i])
        {
          return false;
        }
    }

  return true;
}
