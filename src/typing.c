#include "typing.h"
#include "dev/errors.h"
#include "intf/mm.h"
#include "sds.h"
#include "types.h"
#include <stdio.h>

DEFINE_DBG_ASSERT_I (type_t, type_t, t)
{
  ASSERT (t);
  ASSERT (*t <= T_SARRAY && *t >= T_PRIM);
}

DEFINE_DBG_ASSERT_I (prim_t, prim_t, p)
{
  ASSERT (p);
  ASSERT (*p <= BIT && *p >= U8);
}

u64
prim_bits_size (prim_t t)
{
  prim_t_assert (&t);
  switch (t)
    {
    case BIT:
      return 1;

    case BOOL:
    case U8:
    case I8:
      return 8;

    case U16:
    case I16:
    case F16:
    case CI16:
    case CU16:
      return 16;

    case U32:
    case I32:
    case F32:
    case CI32:
    case CU32:
      return 32;

    case U64:
    case I64:
    case F64:
    case CF64:
    case CI64:
    case CU64:
      return 64;

    case F128:
    case CF128:
    case CI128:
    case CU128:
      return 128;

    case CF256:
      return 256;
    }

  ASSERT (0);
  return 0;
}

const char *
prim_to_str (prim_t p)
{
  switch (p)
    {
    case U8:
      return "U8";
    case U16:
      return "U16";
    case U32:
      return "U32";
    case U64:
      return "U64";

    case I8:
      return "I8";
    case I16:
      return "I16";
    case I32:
      return "I32";
    case I64:
      return "I64";

    case F16:
      return "F16";
    case F32:
      return "F32";
    case F64:
      return "F64";
    case F128:
      return "F128";

    case CF64:
      return "CF64";
    case CF128:
      return "CF128";
    case CF256:
      return "CF256";

    case CI16:
      return "CI16";
    case CI32:
      return "CI32";
    case CI64:
      return "CI64";
    case CI128:
      return "CI128";

    case CU16:
      return "CU16";
    case CU32:
      return "CU32";
    case CU64:
      return "CU64";
    case CU128:
      return "CU128";

    case BOOL:
      return "BOOL";
    case BIT:
      return "BIT";
    }
  prim_t_assert (&p);
  return "";
}

//////////////////////////////// Struct

DEFINE_DBG_ASSERT_I (struct_t, struct_t, s)
{
  ASSERT (s);
  ASSERT (s->len > 0);
  ASSERT (s->keys);
  ASSERT (s->types);
  for (u32 i = 0; i < s->len; ++i)
    {
      type_assert (&s->types[i]);
    }
  ASSERT (strings_all_unique (s->keys, s->len));
}

/**
DEFINE_DBG_ASSERT_I (struct_subset_t, struct_subset_t, s)
{
  ASSERT (s);
  ASSERT (s->len > 0);
  ASSERT (s->lsel > 0);
  ASSERT (s->keys);
  ASSERT (s->types);
  for (u32 i = 0; i < s->len; ++i)
    {
      string_assert (&s->keys[i]);
      type_subset_assert (&s->types[i]);
    }
  for (u32 i = 0; i < s->lsel; ++i)
    {
      ASSERT (s->sel[i] < s->len);
    }
  ASSERT (strings_all_unique (s->keys, s->len));
}
*/

err_t
struct_t_bits_size (u64 *dest, struct_t *t)
{
  ASSERT (dest);
  struct_t_assert (t);

  u64 ret = 0;
  for (u32 i = 0; i < t->len; ++i)
    {
      u64 tsize = 0;
      err_t err = type_bits_size (&tsize, &t->types[i]);
      if (err)
        {
          return err;
        }

      if (U32_MAX - ret > tsize)
        {
          return ERR_OVERFLOW;
        }
      ret += tsize;
    }

  *dest = ret;
  return SUCCESS;
}

//////////////////////////////// Strict Array

DEFINE_DBG_ASSERT_I (sarray_t, sarray_t, s)
{
  ASSERT (s);
  ASSERT (s->rank > 0);
  ASSERT (s->dims);
  type_assert (s->t);
  for (u32 i = 0; i < s->rank; ++i)
    {
      ASSERT (s->dims[i] > 0);
    }
}

/**
DEFINE_DBG_ASSERT_I (sarray_subset_t, sarray_subset_t, s)
{
  ASSERT (s);
  ASSERT (s->rank > 0);
  ASSERT (s->dims);
  type_subset_assert (s->t);
  for (u32 i = 0; i < s->rank; ++i)
    {
      ASSERT (s->dims[i] > 0);
      array_range_assert (&s->ranges[i]);
      ASSERT (s->ranges[i].stop <= s->dims[i]);
    }
}
*/

err_t
sarray_t_bits_size (u64 *dest, sarray_t *sa)
{
  sarray_t_assert (sa);

  if (sa->t->type == T_PRIM && sa->t->p == BIT)
    {
      return sarray_t_bits_size (dest, sa);
    }

  // Get the size of the underlying type
  u64 tsize = 1;
  err_t err = type_bits_size (&tsize, sa->t);
  if (err)
    {
      return err;
    }

  u64 ret = tsize;
  for (u32 i = 0; i < sa->rank; ++i)
    {
      ASSERT (ret != 0);

      // Make clang-tidy happy
#ifdef __clang_analyzer__
      if (ret == 0)
        {
          return -1;
        }
#endif

      if (U32_MAX / ret > sa->dims[i])
        {
          return ERR_OVERFLOW;
        }
      ret *= sa->dims[i];
    }

  *dest = ret;
  return SUCCESS;
}

//////////////////////////////// Generic

DEFINE_DBG_ASSERT_I (type, type, t)
{
  ASSERT (t);
  type_t_assert (&t->type);
  switch (t->type)
    {
    case T_PRIM:
      prim_t_assert (&t->p);
      break;
    case T_STRUCT:
      struct_t_assert (t->st);
      break;
    case T_SARRAY:
      sarray_t_assert (t->sa);
      break;
    default:
      break; // TODO
    }
}

/**
DEFINE_DBG_ASSERT_I (type_subset, type_subset, t)
{
  ASSERT (t);
  type_t_assert (&t->type);
  switch (t->type)
    {
    case T_PRIM:
      prim_t_assert (&t->p);
      break;
    case T_STRUCT:
      struct_subset_t_assert (t->st);
      break;
    case T_SARRAY:
      sarray_subset_t_assert (t->sa);
      break;
    }
}
*/

err_t
type_bits_size (u64 *dest, const type *t)
{
  type_assert (t);

  switch (t->type)
    {
    case T_PRIM:
      *dest = prim_bits_size (t->p);
      return SUCCESS;
    case T_STRUCT:
      return struct_t_bits_size (dest, t->st);
    case T_SARRAY:
      return sarray_t_bits_size (dest, t->sa);
    default:
      ASSERT (0);
    }
  ASSERT (0);
  return 0;
}

void
i_log_type_recurse (type *t)
{
  type_assert (t);

  switch (t->type)
    {
    case T_PRIM:
      {
        fprintf (stderr, " %s ", prim_to_str (t->p));
        break;
      }
    case T_STRUCT:
      {
        fprintf (stderr, "struct { ");
        for (u32 i = 0; i < t->st->len; ++i)
          {
            const string str = t->st->keys[i];
            fprintf (stderr, "%.*s ", str.len, str.data);
            i_log_type_recurse (&t->st->types[i]);
          }
        fprintf (stderr, "} ");
        break;
      }
    case T_UNION:
      {
        fprintf (stderr, "struct { ");
        for (u32 i = 0; i < t->st->len; ++i)
          {
            const string str = t->st->keys[i];
            fprintf (stderr, "%.*s ", str.len, str.data);
            i_log_type_recurse (&t->st->types[i]);
          }
        fprintf (stderr, "} ");
        break;
      }
    case T_ENUM:
      {
        fprintf (stderr, "struct { ");
        for (u32 i = 0; i < t->st->len; ++i)
          {
            const string str = t->st->keys[i];
            fprintf (stderr, "%.*s ", str.len, str.data);
            i_log_type_recurse (&t->st->types[i]);
          }
        fprintf (stderr, "} ");
        break;
      }
    case T_VARRAY:
      {
        ASSERT (0);
        return;
      }
    case T_SARRAY:
      {
        ASSERT (0);
        return;
      }
    }
  return;
}

void
i_log_type (type *t)
{
  i_log_type_recurse (t);
  fprintf (stderr, "\n");
}
