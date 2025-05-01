#include "typing.h"
#include "dev/errors.h"
#include "dev/testing.h"
#include "intf/logging.h"
#include "intf/mm.h"
#include "intf/stdlib.h"
#include "intf/types.h"
#include "utils/bounds.h"
#include <stdlib.h>

//////////////////////////////// Asserts
DEFINE_DBG_ASSERT_I (type, type, t);

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

DEFINE_DBG_ASSERT_I (union_t, union_t, u)
{
  ASSERT (u);
  ASSERT (u->len > 0);
  ASSERT (u->keys);
  ASSERT (u->types);
  for (u32 i = 0; i < u->len; ++i)
    {
      type_assert (&u->types[i]);
    }
  ASSERT (strings_all_unique (u->keys, u->len));
}

DEFINE_DBG_ASSERT_I (enum_t, enum_t, e)
{
  ASSERT (e);
  ASSERT (e->len > 0);
  ASSERT (e->keys);
  for (u32 i = 0; i < e->len; ++i)
    {
      ASSERT (e->keys[i].len > 0);
    }
  ASSERT (strings_all_unique (e->keys, e->len));
}

DEFINE_DBG_ASSERT_I (varray_t, varray_t, v)
{
  ASSERT (v);
  ASSERT (v->rank > 0);
}

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
      struct_t_assert (&t->st);
      break;
    case T_SARRAY:
      sarray_t_assert (&t->sa);
      break;
    default:
      break; // TODO
    }
}

///////////////////////////// FREE
static inline void
_free_struct (type *t, lalloc *alloc)
{
  type_assert (t);
  ASSERT (&t->st);
  for (u32 i = 0; i < t->st.len; ++i)
    {
      ASSERT (t->st.keys[i].data);
      lfree (alloc, t->st.keys[i].data);
      type_free_internals (&t->st.types[i], alloc);
    }
  ASSERT (t->st.keys && t->st.types);
  lfree (alloc, t->st.keys);
  lfree (alloc, t->st.types);
}

static inline void
_free_union (type *t, lalloc *alloc)
{
  type_assert (t);
  ASSERT (&t->un);
  for (u32 i = 0; i < t->un.len; ++i)
    {
      ASSERT (t->un.keys[i].data);
      lfree (alloc, t->un.keys[i].data);
      type_free_internals (&t->un.types[i], alloc);
    }
  ASSERT (t->un.keys && t->un.types);
  lfree (alloc, t->un.keys);
  lfree (alloc, t->un.types);
}

static inline void
_free_enum (type *t, lalloc *alloc)
{
  type_assert (t);
  ASSERT (&t->en);
  for (u32 i = 0; i < t->en.len; ++i)
    {
      ASSERT (t->en.keys[i].data);
      lfree (alloc, t->en.keys[i].data);
    }
  ASSERT (t->en.keys);
  lfree (alloc, t->en.keys);
}

static inline void
_free_sarray (type *t, lalloc *alloc)
{
  type_assert (t);
  ASSERT (t->sa.dims && t->sa.t);
  lfree (alloc, t->sa.dims);
  type_free_internals (t->sa.t, alloc);
}

static inline void
_free_varray (type *t, lalloc *alloc)
{
  type_assert (t);
  ASSERT (t->va.t);
  type_free_internals (t->va.t, alloc);
}

void
type_free_internals (type *t, lalloc *alloc)
{
  type_assert (t);
  ASSERT (alloc);
  switch (t->type)
    {
    case T_PRIM:
      {
        break;
      }
    case T_STRUCT:
      {
        _free_struct (t, alloc);
        break;
      }
    case T_UNION:
      {
        _free_union (t, alloc);
        break;
      }
    case T_ENUM:
      {
        _free_enum (t, alloc);
        break;
      }
    case T_SARRAY:
      {
        _free_sarray (t, alloc);
        break;
      }
    case T_VARRAY:
      {
        _free_varray (t, alloc);
        break;
      }
    default:
      {
        ASSERT (0);
      }
    }
}

//////////////////////////////////////// BITS SIZE
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
    case CF32:
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

    case CF32:
      return "CF32";
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

err_t
struct_t_bits_size (u64 *dest, const struct_t *t)
{
  ASSERT (dest);
  struct_t_assert (t);

  u64 ret = 0;
  err_t err;

  for (u32 i = 0; i < t->len; ++i)
    {
      u64 tsize = 0;
      if ((err = type_bits_size (&tsize, &t->types[i])))
        {
          return err;
        }

      if (!can_add_u64 (ret, tsize))
        {
          return ERR_ARITH;
        }
      ret += tsize;
    }

  *dest = ret;
  return SUCCESS;
}

TEST (struct_t_bits_size)
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
        .p = BOOL,
    },
    {
        .type = T_PRIM,
        .p = U16,
    },
    {
        .type = T_PRIM,
        .p = BIT,
    },
  };
  u64 act;
  u64 exp = (sizeof (u32) + sizeof (u8) + sizeof (u16)) * 8 + 1;
  test_fail_if (struct_t_bits_size (&act, &st));
  test_assert_int_equal (exp, act);
}

//////////////////////////////////// LOG TYPE

#include <stdio.h> // TODO - switch to log

static inline void i_log_type_internal (const type *t);

static inline void
i_log_primitive (prim_t p)
{
  prim_t_assert (&p);
  fprintf (stdout, "%s", prim_to_str (p));
}

static inline void
i_log_struct (const struct_t *st)
{
  struct_t_assert (st);

  fprintf (stdout, "struct { ");
  fprintf (stdout, "%.*s ", st->keys[0].len, st->keys[0].data);
  i_log_type_internal (&st->types[0]);

  for (u32 i = 0; i < st->len; ++i)
    {
      fprintf (stdout, ",");
      fprintf (stdout, "%.*s ", st->keys[i].len, st->keys[i].data);
      i_log_type_internal (&st->types[i]);
    }
  fprintf (stdout, "}");
}

static inline void
i_log_union (const union_t *un)
{
  union_t_assert (un);

  fprintf (stdout, "union { ");
  fprintf (stdout, "%.*s ", un->keys[0].len, un->keys[0].data);
  i_log_type_internal (&un->types[0]);

  for (u32 i = 0; i < un->len; ++i)
    {
      fprintf (stdout, ",");
      fprintf (stdout, "%.*s ", un->keys[i].len, un->keys[i].data);
      i_log_type_internal (&un->types[i]);
    }
  fprintf (stdout, "}");
}

static inline void
i_log_enum (const enum_t *en)
{
  enum_t_assert (en);

  fprintf (stdout, "enum { ");
  fprintf (stdout, "%.*s ", en->keys[0].len, en->keys[0].data);

  for (u32 i = 0; i < en->len; ++i)
    {
      fprintf (stdout, ",");
      fprintf (stdout, "%.*s ", en->keys[i].len, en->keys[i].data);
    }
  fprintf (stdout, "}");
}

static inline void
i_log_varray (const varray_t *va)
{
  varray_t_assert (va);

  fprintf (stdout, "{ ");
  for (u32 i = 0; i < va->rank; ++i)
    {
      fprintf (stdout, "[]");
    }
  i_log_type_internal (va->t);
}

static inline void
i_log_sarray (const sarray_t *sa)
{
  sarray_t_assert (sa);

  fprintf (stdout, "{ ");
  for (u32 i = 0; i < sa->rank; ++i)
    {
      fprintf (stdout, "[%d]", sa->dims[i]);
    }
  i_log_type_internal (sa->t);
}

static inline void
i_log_type_internal (const type *t)
{
  type_assert (t);
  switch (t->type)
    {
    case T_STRUCT:
      i_log_struct (&t->st);
      break;
    case T_UNION:
      i_log_union (&t->un);
      break;
    case T_PRIM:
      i_log_primitive (t->p);
      break;
    case T_VARRAY:
      i_log_varray (&t->va);
      break;
    case T_SARRAY:
      i_log_sarray (&t->sa);
      break;
    case T_ENUM:
      i_log_enum (&t->en);
      break;
    }
}

void
i_log_type (const type *t)
{
  i_log_type_internal (t);
  fprintf (stdout, "\n");
}

//////////////////////////////// Strict Array

err_t
sarray_t_bits_size (u64 *dest, const sarray_t *sa)
{
  ASSERT (dest);
  sarray_t_assert (sa);

  // Get the size of the underlying type
  u64 ret;
  err_t err = type_bits_size (&ret, sa->t);
  if (err)
    {
      return err;
    }

  for (u32 i = 0; i < sa->rank; ++i)
    {
      ASSERT (ret != 0);

      if (!can_mul_u64 (ret, sa->dims[i]))
        {
          return ERR_ARITH;
        }
      ret *= (u64)sa->dims[i];
    }

  *dest = ret;
  return SUCCESS;
}

//////////////////////////////// Generic

err_t
type_bits_size (u64 *dest, const type *t)
{
  type_assert (t);

  switch (t->type)
    {
    case T_PRIM:
      {
        *dest = prim_bits_size (t->p);
        return SUCCESS;
      }
    case T_STRUCT:
      {
        return struct_t_bits_size (dest, &t->st);
      }
    case T_SARRAY:
      {
        return sarray_t_bits_size (dest, &t->sa);
      }
    default:
      {
        ASSERT (0);
      }
    }
  ASSERT (0);
  return ERR_FALLBACK;
}

//////////////////////////////// Get Type Serializable Size

/**
 * TODO - Before release, all this serialization junk
 * needs to be refactored. It's a mess. Here's the start:
typedef struct
{
  type *t;
  u8 *data;
  u16 dlen;
  u16 idx;
} type_ser;
*/

static inline err_t
ser_size_add (u16 *size, u16 inc)
{
  if (!can_add_u16 (*size, inc))
    {
      return ERR_ARITH;
    }
  *size += inc;
  return SUCCESS;
}

static inline err_t
ser_size_struct (u16 *size, const struct_t *st)
{
  err_t ret;

  // write number of fields
  if ((ret = ser_size_add (size, sizeof (st->len))))
    {
      return ret;
    }

  for (u32 i = 0; i < st->len; ++i)
    {
      // key length
      if ((ret = ser_size_add (size, sizeof (st->keys[i].len))))
        {
          return ret;
        }

      // key bytes
      if ((ret = ser_size_add (size, st->keys[i].len)))
        {
          return ret;
        }

      // nested type
      {
        u16 sub;
        if ((ret = type_get_serial_size (&sub, &st->types[i])))
          {
            return ret;
          }
        if ((ret = ser_size_add (size, sub)))
          {
            return ret;
          }
      }
    }

  return SUCCESS;
}

TEST (ser_size_struct)
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
        .p = BOOL,
    },
    {
        .type = T_PRIM,
        .p = U16,
    },
    {
        .type = T_PRIM,
        .p = U8,
    },
  };
  u16 one_type = sizeof (u8) + sizeof (u8);

  // Expected
  u16 exp = 0;
  exp += sizeof (u16);                     // Len
  exp += 4 * sizeof (u16) + 3 + 2 + 4 + 5; // String keys
  exp += 4 * one_type;                     // Type values

  u16 act;
  ser_size_struct (&act, &st);

  test_assert_int_equal (exp, act);
}

static inline err_t
ser_size_union (u16 *size, const union_t *un)
{
  err_t ret;

  // write number of variants
  if ((ret = ser_size_add (size, sizeof (un->len))))
    {
      return ret;
    }

  for (u32 i = 0; i < un->len; ++i)
    {
      // key length
      if ((ret = ser_size_add (size, sizeof (un->keys[i].len))))
        {
          return ret;
        }

      // key bytes
      if ((ret = ser_size_add (size, un->keys[i].len)))
        {
          return ret;
        }

      // nested type
      {
        u16 sub;
        if ((ret = type_get_serial_size (&sub, &un->types[i])))
          {
            return ret;
          }
        if ((ret = ser_size_add (size, sub)))
          {
            return ret;
          }
      }
    }

  return SUCCESS;
}

TEST (ser_size_union)
{
  union_t un;
  un.len = 4;
  un.keys = (string[]){
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
  un.types = (type[]){
    {
        .type = T_PRIM,
        .p = U32,
    },
    {
        .type = T_PRIM,
        .p = BOOL,
    },
    {
        .type = T_PRIM,
        .p = U16,
    },
    {
        .type = T_PRIM,
        .p = U8,
    },
  };
  u16 one_type = sizeof (u8) + sizeof (u8);

  // Expected
  u16 exp = 0;
  exp += sizeof (u16);                     // Len
  exp += 4 * sizeof (u16) + 3 + 2 + 4 + 5; // String keys
  exp += 4 * one_type;                     // Type values

  u16 act;
  ser_size_union (&act, &un);

  test_assert_int_equal (exp, act);
}

static inline err_t
ser_size_enum (u16 *size, const enum_t *en)
{
  err_t ret;

  // write number of entries
  if ((ret = ser_size_add (size, sizeof (en->len))))
    {
      return ret;
    }

  for (u32 i = 0; i < en->len; ++i)
    {
      // key length
      if ((ret = ser_size_add (size, sizeof (en->keys[i].len))))
        {
          return ret;
        }

      // key bytes
      if ((ret = ser_size_add (size, en->keys[i].len)))
        {
          return ret;
        }
    }

  return SUCCESS;
}

TEST (ser_size_enum)
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

  u16 act;
  ser_size_enum (&act, &en);

  test_assert_int_equal (exp, act);
}

static inline err_t
ser_size_varray (u16 *size, const varray_t *va)
{
  err_t ret;

  // write rank
  if ((ret = ser_size_add (size, sizeof (va->rank))))
    {
      return ret;
    }

  // nested type
  {
    u16 sub;
    if ((ret = type_get_serial_size (&sub, va->t)))
      {
        return ret;
      }
    if ((ret = ser_size_add (size, sub)))
      {
        return ret;
      }
  }

  return SUCCESS;
}

TEST (ser_size_varray)
{
  varray_t va;
  va.rank = 4;
  va.t = &(type){
    .type = T_PRIM,
    .p = U8,
  };

  // Expected
  u16 exp = 0;
  exp += sizeof (u16);              // Len
  exp += sizeof (u8) + sizeof (u8); // Type values

  u16 act;
  ser_size_varray (&act, &va);

  test_assert_int_equal (exp, act);
}

static inline err_t
ser_size_sarray (u16 *size, const sarray_t *sa)
{
  err_t ret;

  // write rank
  if ((ret = ser_size_add (size, sizeof (sa->rank))))
    {
      return ret;
    }

  // dimensions block
  {
    u16 dims_size = sizeof (*sa->dims) * sa->rank;
    if (!can_mul_u16 (sizeof (*sa->dims), sa->rank))
      {
        return ERR_ARITH;
      }
    if ((ret = ser_size_add (size, dims_size)))
      {
        return ret;
      }
  }

  // nested element type
  {
    u16 sub;
    if ((ret = type_get_serial_size (&sub, sa->t)))
      {
        return ret;
      }
    if ((ret = ser_size_add (size, sub)))
      {
        return ret;
      }
  }

  return SUCCESS;
}

TEST (ser_size_sarray)
{
  sarray_t sa;
  sa.rank = 4;
  sa.dims = (u32[]){ 10, 20, 30, 40 };
  sa.t = &(type){
    .type = T_PRIM,
    .p = U8,
  };

  // Expected
  u16 exp = 0;
  exp += sizeof (u16);              // Len
  exp += 4 * sizeof (u32);          // Dims
  exp += sizeof (u8) + sizeof (u8); // Type salues

  u16 act;
  ser_size_sarray (&act, &sa);

  test_assert_int_equal (exp, act);
}

//////////////////////////////////// Serialization

err_t
type_get_serial_size (u16 *dest, const type *t)
{
  type_assert (t);
  ASSERT (dest);

  u16 size = 1; // initial tag byte
  err_t ret;

  switch (t->type)
    {
    case T_PRIM:
      {
        // tag + subtype byte
        ret = ser_size_add (&size, 1);
        break;
      }

    case T_STRUCT:
      {
        ret = ser_size_struct (&size, &t->st);
        break;
      }

    case T_UNION:
      {
        ret = ser_size_union (&size, &t->un);
        break;
      }

    case T_ENUM:
      {
        ret = ser_size_enum (&size, &t->en);
        break;
      }

    case T_VARRAY:
      {
        ret = ser_size_varray (&size, &t->va);
        break;
      }

    case T_SARRAY:
      {
        ret = ser_size_sarray (&size, &t->sa);
        break;
      }

    default:
      {
        ASSERT (0);
        return ERR_FALLBACK;
      }
    }

  if (ret)
    {
      return ret;
    }

  *dest = size;
  return SUCCESS;
}

TEST (type_get_serial_size)
{
  // Oooooof
  type t = {
    .type = T_STRUCT,
    .st = (struct_t){
        .len = 2,
        .keys = (string[]){
            { .len = 3, .data = "foo" },
            { .len = 3, .data = "bar" },
        },
        .types = (type[]){
            // Field 0: UNION
            {
                .type = T_UNION,
                .un = (union_t){
                    .len = 2,
                    .keys = (string[]){
                        { .len = 2, .data = "a0" },
                        { .len = 2, .data = "a1" },
                    },
                    .types = (type[]){
                        // Variant 0: PRIM (U8)
                        {
                            .type = T_PRIM,
                            .p = U8,
                        },
                        // Variant 1: VARRAY of PRIM (BOOL)
                        {
                            .type = T_VARRAY,
                            .va = (varray_t){
                                .rank = 2,
                                .t = &(type){
                                    .type = T_PRIM,
                                    .p = BOOL,
                                },
                            },
                        },
                    },
                },
            },
            // Field 1: SARRAY
            {
                .type = T_SARRAY,
                .sa = (sarray_t){
                    .rank = 2,
                    .dims = (u32[]){ 3, 5 },
                    .t = &(type){
                        .type = T_ENUM,
                        .en = (enum_t){
                            .len = 2,
                            .keys = (string[]){
                                { .len = 2, .data = "e0" },
                                { .len = 2, .data = "e1" },
                            },
                        },
                    },
                },
            },
        },
    },
  };

  // Manually calculate expected serialized size

  u16 exp = 0;
  exp += 1; // Root tag (T_STRUCT)

  // STRUCT
  exp += sizeof (u16); // struct len = 2

  // Field 0: key "foo"
  exp += sizeof (u16) + 3; // key length + key bytes
  exp += 1;                // UNION tag
  exp += sizeof (u16);     // union len = 2

  // UNION Variant 0: "a0"
  exp += sizeof (u16) + 2; // key length + key bytes
  exp += 1;                // PRIM tag
  exp += 1;                // PRIM subtype

  // UNION Variant 1: "a1"
  exp += sizeof (u16) + 2; // key length + key bytes
  exp += 1;                // VARRAY tag
  exp += sizeof (u16);     // rank
  exp += 1;                // PRIM tag
  exp += 1;                // PRIM subtype

  // Field 1: key "bar"
  exp += sizeof (u16) + 3; // key length + key bytes
  exp += 1;                // SARRAY tag
  exp += sizeof (u16);     // rank
  exp += 2 * sizeof (u32); // 2 dims
  exp += 1;                // ENUM tag
  exp += sizeof (u16);     // enum len = 2

  // ENUM variant 0: "e0"
  exp += sizeof (u16) + 2;

  // ENUM variant 1: "e1"
  exp += sizeof (u16) + 2;

  u16 act;
  err_t r = type_get_serial_size (&act, &t);
  test_assert_int_equal (r, SUCCESS);
  test_assert_int_equal (exp, act);
}

//////////////////////////////// Serialize

// Recursive implementations
static err_t _type_serialize (type_serialize_params params, u16 *idx);
static err_t _type_deserialize (type_deserialize_params params, u16 *idx);

static inline err_t
ser_write_bytes (u8 *dest, u16 dlen, u16 *idx, const void *src, u16 len)
{
  if (len > dlen)
    {
      return ERR_INVALID_STATE;
    }
  if (*idx > dlen - len)
    {
      return ERR_INVALID_STATE;
    }
  i_memcpy (dest + *idx, src, len);
  *idx += len;
  return SUCCESS;
}

static inline err_t
ser_write_u8 (u8 *dest, u16 dlen, u16 *idx, u8 val)
{
  return ser_write_bytes (dest, dlen, idx, &val, sizeof (val));
}

static inline err_t
ser_write_u16 (u8 *dest, u16 dlen, u16 *idx, u16 val)
{
  return ser_write_bytes (dest, dlen, idx, &val, sizeof (val));
}

static inline err_t
_type_serialize_prim (type_serialize_params p, u16 *idx)
{
  // write the subtype byte
  return ser_write_u8 (p.dest, p.dlen, idx, p.src->p);
}

static inline err_t
_type_serialize_struct (type_serialize_params p, u16 *idx)
{
  err_t ret;
  struct_t *st = &p.src->st;

  // number of fields
  if ((ret = ser_write_u16 (p.dest, p.dlen, idx, st->len)))
    {
      return ret;
    }

  for (u32 i = 0; i < st->len; ++i)
    {
      // key length + key bytes
      if ((ret = ser_write_u16 (p.dest, p.dlen, idx, st->keys[i].len)))
        {
          return ret;
        }
      if ((ret = ser_write_bytes (
               p.dest, p.dlen, idx,
               st->keys[i].data,
               st->keys[i].len)))
        {
          return ret;
        }

      // nested type
      {
        type_serialize_params subp = {
          .src = &st->types[i],
          .dest = p.dest,
          .dlen = p.dlen,
        };
        if ((ret = _type_serialize (subp, idx)))
          {
            return ret;
          }
      }
    }

  return SUCCESS;
}

TEST (_type_serialize_struct)
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
        .p = BOOL,
    },
    {
        .type = T_PRIM,
        .p = U16,
    },
    {
        .type = T_PRIM,
        .p = U8,
    },
  };

  u16 size;
  ser_size_struct (&size, &st);
  u8 *output = malloc (size);
  test_fail_if_null (output);

  type t = {
    .type = T_STRUCT,
    .st = st,
  };

  type_serialize_params params = {
    .dest = output,
    .src = &t,
    .dlen = size,
  };
  u16 idx = 0;
  test_fail_if (_type_serialize_struct (params, &idx));
  test_assert_int_equal (idx, size);
  test_assert_int_equal (idx, 32);

  // Length
  test_assert_int_equal (*(u16 *)&output[0], 4);

  // Field 1
  test_assert_int_equal (*(u16 *)&output[2], 3);
  test_assert_int_equal (i_strncmp ((char *)&output[4], "foo", 3), 0);
  test_assert_int_equal (*(u8 *)&output[7], (u8)T_PRIM);
  test_assert_int_equal (*(u8 *)&output[8], (u8)U32);

  // Field 2
  test_assert_int_equal (*(u16 *)&output[9], 2);
  test_assert_int_equal (i_strncmp ((char *)&output[11], "fo", 2), 0);
  test_assert_int_equal (*(u8 *)&output[13], (u8)T_PRIM);
  test_assert_int_equal (*(u8 *)&output[14], (u8)BOOL);

  // Field 3
  test_assert_int_equal (*(u16 *)&output[15], 4);
  test_assert_int_equal (i_strncmp ((char *)&output[17], "baro", 4), 0);
  test_assert_int_equal (*(u8 *)&output[21], (u8)T_PRIM);
  test_assert_int_equal (*(u8 *)&output[22], (u8)U16);

  // Field 4
  test_assert_int_equal (*(u16 *)&output[23], 5);
  test_assert_int_equal (i_strncmp ((char *)&output[25], "bazbi", 5), 0);
  test_assert_int_equal (*(u8 *)&output[30], (u8)T_PRIM);
  test_assert_int_equal (*(u8 *)&output[31], (u8)U8);
  free (output);
}

static inline err_t
_type_serialize_union (type_serialize_params p, u16 *idx)
{
  err_t ret;
  union_t *un = &p.src->un;

  // number of variants
  if ((ret = ser_write_u16 (p.dest, p.dlen, idx, un->len)))
    {
      return ret;
    }

  for (u32 i = 0; i < un->len; ++i)
    {
      // key length + key bytes
      if ((ret = ser_write_u16 (p.dest, p.dlen, idx, un->keys[i].len)))
        {
          return ret;
        }
      if ((ret = ser_write_bytes (
               p.dest, p.dlen, idx,
               un->keys[i].data,
               un->keys[i].len)))
        {
          return ret;
        }

      // nested type
      {
        type_serialize_params subp = {
          .src = &un->types[i],
          .dest = p.dest,
          .dlen = p.dlen,
        };
        if ((ret = _type_serialize (subp, idx)))
          {
            return ret;
          }
      }
    }

  return SUCCESS;
}

TEST (_type_serialize_union)
{
  union_t un;
  un.len = 4;
  un.keys = (string[]){
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
  un.types = (type[]){
    {
        .type = T_PRIM,
        .p = U32,
    },
    {
        .type = T_PRIM,
        .p = BOOL,
    },
    {
        .type = T_PRIM,
        .p = U16,
    },
    {
        .type = T_PRIM,
        .p = U8,
    },
  };

  u16 size;
  ser_size_union (&size, &un);
  u8 *output = malloc (size);
  test_fail_if_null (output);

  type t = {
    .type = T_STRUCT,
    .un = un,
  };

  type_serialize_params params = {
    .dest = output,
    .src = &t,
    .dlen = size,
  };
  u16 idx = 0;
  test_fail_if (_type_serialize_union (params, &idx));
  test_assert_int_equal (idx, size);
  test_assert_int_equal (idx, 32);

  test_assert_int_equal (*(u16 *)&output[0], 4);

  // Field 1
  test_assert_int_equal (*(u16 *)&output[2], 3);
  test_assert_int_equal (i_strncmp ((char *)&output[4], "foo", 3), 0);
  test_assert_int_equal (*(u8 *)&output[7], (u8)T_PRIM);
  test_assert_int_equal (*(u8 *)&output[8], (u8)U32);

  // Field 2
  test_assert_int_equal (*(u16 *)&output[9], 2);
  test_assert_int_equal (i_strncmp ((char *)&output[11], "fo", 2), 0);
  test_assert_int_equal (*(u8 *)&output[13], (u8)T_PRIM);
  test_assert_int_equal (*(u8 *)&output[14], (u8)BOOL);

  // Field 3
  test_assert_int_equal (*(u16 *)&output[15], 4);
  test_assert_int_equal (i_strncmp ((char *)&output[17], "baro", 4), 0);
  test_assert_int_equal (*(u8 *)&output[21], (u8)T_PRIM);
  test_assert_int_equal (*(u8 *)&output[22], (u8)U16);

  // Field 4
  test_assert_int_equal (*(u16 *)&output[23], 5);
  test_assert_int_equal (i_strncmp ((char *)&output[25], "bazbi", 5), 0);
  test_assert_int_equal (*(u8 *)&output[30], (u8)T_PRIM);
  test_assert_int_equal (*(u8 *)&output[31], (u8)U8);
  free (output);
}

static inline err_t
_type_serialize_enum (type_serialize_params p, u16 *idx)
{
  err_t ret;
  enum_t *en = &p.src->en;

  // number of entries
  if ((ret = ser_write_u16 (p.dest, p.dlen, idx, en->len)))
    {
      return ret;
    }

  for (u32 i = 0; i < en->len; ++i)
    {
      // key length + key bytes
      if ((ret = ser_write_u16 (p.dest, p.dlen, idx, en->keys[i].len)))
        {
          return ret;
        }
      if ((ret = ser_write_bytes (
               p.dest, p.dlen, idx,
               en->keys[i].data,
               en->keys[i].len)))
        {
          return ret;
        }
    }

  return SUCCESS;
}

TEST (_type_serialize_enum)
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

  u16 size;
  ser_size_enum (&size, &en);
  u8 *output = malloc (size);
  test_fail_if_null (output);

  type t = {
    .type = T_STRUCT,
    .en = en,
  };

  type_serialize_params params = {
    .dest = output,
    .src = &t,
    .dlen = size,
  };
  u16 idx = 0;
  test_fail_if (_type_serialize_enum (params, &idx));
  test_assert_int_equal (idx, size);
  test_assert_int_equal (idx, 24);

  test_assert_int_equal (*(u16 *)&output[0], 4);

  // Field 1
  test_assert_int_equal (*(u16 *)&output[2], 3);
  test_assert_int_equal (i_strncmp ((char *)&output[4], "foo", 3), 0);

  // Field 2
  test_assert_int_equal (*(u16 *)&output[7], 2);
  test_assert_int_equal (i_strncmp ((char *)&output[9], "fo", 2), 0);

  // Field 3
  test_assert_int_equal (*(u16 *)&output[11], 4);
  test_assert_int_equal (i_strncmp ((char *)&output[13], "baro", 4), 0);

  // Field 4
  test_assert_int_equal (*(u16 *)&output[17], 5);
  test_assert_int_equal (i_strncmp ((char *)&output[19], "bazbi", 5), 0);
  free (output);
}

static inline err_t
_type_serialize_varray (type_serialize_params p, u16 *idx)
{
  err_t ret;
  varray_t *va = &p.src->va;

  // rank
  if ((ret = ser_write_u16 (p.dest, p.dlen, idx, va->rank)))
    {
      return ret;
    }

  // nested element type
  {
    type_serialize_params subp = {
      .src = va->t,
      .dest = p.dest,
      .dlen = p.dlen,
    };
    if ((ret = _type_serialize (subp, idx)))
      {
        return ret;
      }
  }

  return SUCCESS;
}

TEST (_type_serialize_varray)
{
  varray_t va;
  va.rank = 4;
  va.t = &(type){
    .type = T_PRIM,
    .p = U8,
  };

  u16 size;
  ser_size_varray (&size, &va);
  u8 *output = malloc (size);
  test_fail_if_null (output);

  type t = {
    .type = T_STRUCT,
    .va = va,
  };

  type_serialize_params params = {
    .dest = output,
    .src = &t,
    .dlen = size,
  };
  u16 idx = 0;
  test_fail_if (_type_serialize_varray (params, &idx));
  test_assert_int_equal (idx, size);
  test_assert_int_equal (idx, 4);

  // Rank
  test_assert_int_equal (*(u16 *)&output[0], 4);

  // Data type
  test_assert_int_equal (*(u8 *)&output[2], (u8)T_PRIM);
  test_assert_int_equal (*(u8 *)&output[3], (u8)U8);
  free (output);
}

static inline err_t
_type_serialize_sarray (type_serialize_params p, u16 *idx)
{
  err_t ret;
  sarray_t *sa = &p.src->sa;

  // rank
  if ((ret = ser_write_u16 (p.dest, p.dlen, idx, sa->rank)))
    {
      return ret;
    }

  // dimensions block
  {
    u16 dims_size = sa->rank * (u16)sizeof (*sa->dims);
    if (!can_mul_u16 ((u16)sizeof (*sa->dims), sa->rank))
      {
        return ERR_ARITH;
      }
    if ((ret = ser_write_bytes (
             p.dest, p.dlen, idx,
             sa->dims,
             dims_size)))
      {
        return ret;
      }
  }

  // nested element type
  {
    type_serialize_params subp = {
      .src = sa->t,
      .dest = p.dest,
      .dlen = p.dlen,
    };
    if ((ret = _type_serialize (subp, idx)))
      {
        return ret;
      }
  }

  return SUCCESS;
}

TEST (_type_serialize_sarray)
{
  sarray_t sa;
  sa.rank = 4;
  sa.dims = (u32[]){ 10, 11, 25, 30 };
  sa.t = &(type){
    .type = T_PRIM,
    .p = U8,
  };

  u16 size;
  ser_size_sarray (&size, &sa);
  u8 *output = malloc (size);
  test_fail_if_null (output);

  type t = {
    .type = T_STRUCT,
    .sa = sa,
  };

  type_serialize_params params = {
    .dest = output,
    .src = &t,
    .dlen = size,
  };
  u16 idx = 0;
  test_fail_if (_type_serialize_sarray (params, &idx));
  test_assert_int_equal (idx, size);
  test_assert_int_equal (idx, 20);

  // Dims Length
  test_assert_int_equal (*(u16 *)&output[0], 4);

  // Each Dim
  test_assert_int_equal (*(u32 *)&output[2], 10);
  test_assert_int_equal (*(u32 *)&output[6], 11);
  test_assert_int_equal (*(u32 *)&output[10], 25);
  test_assert_int_equal (*(u32 *)&output[14], 30);

  // Data type
  test_assert_int_equal (*(u8 *)&output[18], (u8)T_PRIM);
  test_assert_int_equal (*(u8 *)&output[19], (u8)U8);
  free (output);
}

static err_t
_type_serialize (type_serialize_params params, u16 *idx)
{
  type_assert (params.src);
  ASSERT (params.dest);

  err_t ret;

  // write the type tag
  if ((ret = ser_write_u8 (
           params.dest, params.dlen, idx,
           (u8)params.src->type)))
    {
      return ret;
    }

  switch (params.src->type)
    {
    case T_PRIM:
      {
        return _type_serialize_prim (params, idx);
      }
    case T_STRUCT:
      {
        return _type_serialize_struct (params, idx);
      }
    case T_UNION:
      {
        return _type_serialize_union (params, idx);
      }
    case T_ENUM:
      {
        return _type_serialize_enum (params, idx);
      }
    case T_VARRAY:
      {
        return _type_serialize_varray (params, idx);
      }
    case T_SARRAY:
      {
        return _type_serialize_sarray (params, idx);
      }
    default:
      {
        ASSERT (0);
        return ERR_INVALID_STATE;
      }
    }
}

err_t
type_serialize (type_serialize_params params)
{
  type_assert (params.src);
  ASSERT (params.dest);
  u16 idx = 0;
  return _type_serialize (params, &idx);
}

///////////////// Deserialization

static inline err_t
ser_read_bytes (const u8 *src, u16 dlen, u16 *idx, void *dst, u16 len)
{
  if (len > dlen)
    {
      return ERR_INVALID_STATE;
    }
  if (*idx > dlen - len)
    {
      return ERR_INVALID_STATE;
    }
  i_memcpy (dst, src + *idx, len);
  *idx += len;
  return SUCCESS;
}

static inline err_t
ser_read_u8 (const u8 *src, u16 dlen, u16 *idx, u8 *out)
{
  return ser_read_bytes (src, dlen, idx, out, sizeof (*out));
}

static inline err_t
ser_read_u16 (const u8 *src, u16 dlen, u16 *idx, u16 *out)
{
  return ser_read_bytes (src, dlen, idx, out, sizeof (*out));
}

static inline err_t
_type_deserialize_prim (type_deserialize_params p, u16 *idx)
{
  err_t ret;
  u8 _p = (u8)p.dest->p;
  if ((ret = ser_read_u8 (p.src, p.dlen, idx, &_p)))
    {
      return ret;
    }
  p.dest->p = (prim_t)_p;
  return SUCCESS;
}

// Common code for struct and union (they have
// similar string / type array pattern)
static inline err_t
_deserialize_keys_and_types (
    type_deserialize_params p,
    string **out_keys,
    type **out_types,
    u16 len,
    u16 *idx)
{
  err_t ret;
  string *keys = NULL;
  type *types = NULL;
  i32 i = 0;

  // allocate arrays
  keys = lmalloc (p.type_allocator, len * sizeof (*keys));
  types = lmalloc (p.type_allocator, len * sizeof (*types));
  if (keys == NULL || types == NULL)
    {
      ret = ERR_NOMEM;
      goto failed;
    }

  for (i = 0; i < len; ++i)
    {
      ////////// Deserialize string key
      // Length
      if ((ret = ser_read_u16 (p.src, p.dlen, idx, &keys[i].len)))
        {
          goto failed;
        }

      // Data
      keys[i].data = lmalloc (p.type_allocator, keys[i].len);
      if (keys[i].data == NULL)
        {
          ret = ERR_NOMEM;
          goto failed;
        }
      if ((ret = ser_read_bytes (
               p.src, p.dlen, idx,
               keys[i].data, keys[i].len)))
        {
          goto failed;
        }

      ////////// Deserialize types of field
      type_deserialize_params subp = {
        .dest = &types[i],
        .type_allocator = p.type_allocator,
        .src = p.src,
        .dlen = p.dlen,
      };
      if ((ret = _type_deserialize (subp, idx)))
        {
          lfree (p.type_allocator, keys[i].data);
          keys[i].data = NULL;

          // Assume all functions are
          // good citizens and clean up
          // on failure
          goto failed;
        }
    }

  // assign out pointers on success
  *out_keys = keys;
  *out_types = types;

  return SUCCESS;

failed:
  if (keys && types)
    {
      // Start at the previous one
      // because we freed the top
      for (i = i - 1; i >= 0; --i)
        {
          ASSERT (keys[i].data);
          type_assert (&types[i]);

          lfree (p.type_allocator, keys[i].data);
          type_free_internals (&types[i], p.type_allocator);
        }
    }
  if (keys)
    {
      lfree (p.type_allocator, keys);
    }
  if (types)
    {
      lfree (p.type_allocator, types);
    }

  return ret;
}

static inline err_t
_type_deserialize_struct (type_deserialize_params p, u16 *idx)
{
  err_t ret;
  struct_t st;
  u16 len = 0;

  // Read in length of keys / types
  if ((ret = ser_read_u16 (p.src, p.dlen, idx, &len)))
    {
      return ret;
    }
  st.len = len;

  // Read in keys and types
  if ((ret = _deserialize_keys_and_types (
           p, &st.keys, &st.types, len, idx)))
    {
      return ret;
    }

  p.dest->st = st;
  return SUCCESS;
}

static inline err_t
_type_deserialize_union (type_deserialize_params p, u16 *idx)
{
  err_t ret;
  union_t un;
  u16 len = 0;

  // Read in the len of keys / types
  if ((ret = ser_read_u16 (p.src, p.dlen, idx, &len)))
    {
      return ret;
    }
  un.len = len;

  // Read in keys and types
  if ((ret = _deserialize_keys_and_types (
           p, &un.keys, &un.types, len, idx)))
    {
      return ret;
    }

  p.dest->un = un;
  return SUCCESS;
}

static inline err_t
_type_deserialize_enum (type_deserialize_params p, u16 *idx)
{
  err_t ret;
  enum_t en;
  string *keys = NULL;
  u16 len;
  i32 i = 0;

  // read entry count
  if ((ret = ser_read_u16 (p.src, p.dlen, idx, &len)))
    {
      return ret;
    }

  keys = lmalloc (p.type_allocator, len * sizeof (*keys));
  if (keys == NULL)
    {
      goto failed;
    }

  en.keys = keys;
  en.len = len;

  for (; i < len; ++i)
    {
      // key length
      if ((ret = ser_read_u16 (p.src, p.dlen, idx, &en.keys[i].len)))
        {
          goto failed;
        }

      // allocate & read key data
      en.keys[i].data = lmalloc (
          p.type_allocator,
          en.keys[i].len);

      if (en.keys[i].data == NULL)
        {
          ret = ERR_NOMEM;
          goto failed;
        }

      if ((ret = ser_read_bytes (
               p.src, p.dlen, idx,
               en.keys[i].data,
               en.keys[i].len)))
        {
          lfree (p.type_allocator, en.keys[i].data);
          goto failed;
        }
    }

  p.dest->en = en;
  return SUCCESS;

failed:
  if (keys)
    {
      // i - 1 because top element is already freed on error
      for (i = i - 1; i >= 0; --i)
        {
          ASSERT (en.keys[i].data);
          lfree (p.type_allocator, en.keys[i].data);
        }
      lfree (p.type_allocator, keys);
    }
  return ret;
}

static inline err_t
_type_deserialize_varray (type_deserialize_params p, u16 *idx)
{
  varray_t va;
  type *t = NULL;
  err_t ret = SUCCESS;
  u16 rank;

  // Read the rank from the array
  if ((ret = ser_read_u16 (p.src, p.dlen, idx, &rank)))
    {
      goto failed;
    }

  // Allocate memory
  t = lmalloc (p.type_allocator, sizeof *t);

  // Error check
  if (t == NULL)
    {
      ret = ERR_NOMEM;
      goto failed;
    }

  va.t = t;
  va.rank = rank;

  // Deserialize sub type
  type_deserialize_params subp = {
    .dest = va.t,
    .type_allocator = p.type_allocator,
    .src = p.src,
    .dlen = p.dlen,
  };
  if ((ret = _type_deserialize (subp, idx)))
    {
      goto failed;
    }

  p.dest->va = va;
  return SUCCESS;

failed:
  if (t)
    {
      lfree (p.type_allocator, t);
    }
  return ret;
}

static inline err_t
_type_deserialize_sarray (type_deserialize_params p, u16 *idx)
{
  err_t ret;
  sarray_t sa;
  type *t = NULL;
  u32 *dims = NULL;
  u16 rank;

  // First, parse rank
  if ((ret = ser_read_u16 (p.src, p.dlen, idx, &rank)))
    {
      goto failed;
    }
  if (!can_mul_u16 ((u16)sizeof *sa.dims, rank))
    {
      return ERR_ARITH;
    }

  // Allocate memory
  t = lmalloc (p.type_allocator, sizeof *t);
  dims = lmalloc (p.type_allocator, rank * sizeof *dims);

  // Error check
  if (t == NULL || dims == NULL)
    {
      ret = ERR_NOMEM;
      goto failed;
    }

  sa.t = t;
  sa.dims = dims;
  sa.rank = rank;

  // Read dims array
  if ((ret = ser_read_bytes (
           p.src,
           p.dlen,
           idx,
           sa.dims,
           sa.rank * sizeof *sa.dims)))
    {
      goto failed;
    }

  // TODO - maybe here you could check each
  // of the u32s? Max dim size maybe

  // nested type
  type_deserialize_params subp = {
    .dest = sa.t,
    .type_allocator = p.type_allocator,
    .src = p.src,
    .dlen = p.dlen,
  };
  if ((ret = _type_deserialize (subp, idx)))
    {
      // Trust that sub functions are good citizens
      // and free memory on failure
      goto failed;
    }

  p.dest->sa = sa;
  return SUCCESS;

failed:
  if (t)
    {
      lfree (p.type_allocator, t);
    }
  if (dims)
    {
      lfree (p.type_allocator, dims);
    }
  return ret;
}

static err_t
_type_deserialize (type_deserialize_params params, u16 *idx)
{
  ASSERT (params.src);
  ASSERT (params.type_allocator);

  err_t ret;
  u8 tag;

  // read the type tag
  if ((ret = ser_read_u8 (params.src, params.dlen, idx, &tag)))
    {
      return ret;
    }
  params.dest->type = (type_t)tag;

  switch (params.dest->type)
    {
    case T_PRIM:
      {
        return _type_deserialize_prim (params, idx);
      }
    case T_STRUCT:
      {
        return _type_deserialize_struct (params, idx);
      }
    case T_UNION:
      {
        return _type_deserialize_union (params, idx);
      }
    case T_ENUM:
      {
        return _type_deserialize_enum (params, idx);
      }
    case T_VARRAY:
      {
        return _type_deserialize_varray (params, idx);
      }
    case T_SARRAY:
      {
        return _type_deserialize_sarray (params, idx);
      }
    default:
      {
        return ERR_INVALID_STATE;
      }
    }
}

err_t
type_deserialize (type_deserialize_params params)
{
  type_assert (params.dest);
  ASSERT (params.src);
  ASSERT (params.type_allocator);

  u16 idx = 0;
  err_t ret;
  u8 tag;

  // read the type tag
  if ((ret = ser_read_u8 (params.src, params.dlen, &idx, &tag)))
    {
      return ret;
    }
  params.dest->type = (type_t)tag;

  switch (params.dest->type)
    {
    case T_PRIM:
      {
        return _type_deserialize_prim (params, &idx);
      }
    case T_STRUCT:
      {
        return _type_deserialize_struct (params, &idx);
      }
    case T_UNION:
      {
        return _type_deserialize_union (params, &idx);
      }
    case T_ENUM:
      {
        return _type_deserialize_enum (params, &idx);
      }
    case T_VARRAY:
      {
        return _type_deserialize_varray (params, &idx);
      }
    case T_SARRAY:
      {
        return _type_deserialize_sarray (params, &idx);
      }
    default:
      {
        return ERR_INVALID_STATE;
      }
    }
}

TEST (_type_serialize_deserialize)
{
  lalloc alloc;
  lalloc_create (&alloc, 40960);

  // Primitive type
  type prim = { .type = T_PRIM, .p = U16 };

  // Struct with one field
  type struct_inner = { .type = T_PRIM, .p = U8 };
  struct_t st_val;
  st_val.len = 1;
  st_val.keys = lmalloc (&alloc, sizeof (string));
  st_val.keys[0] = (string){ .len = 3, .data = "key" };
  st_val.types = lmalloc (&alloc, sizeof (type));
  st_val.types[0] = struct_inner;

  type struct_type = { .type = T_STRUCT, .st = st_val };

  // Union with one variant
  type union_inner = { .type = T_PRIM, .p = U32 };
  union_t un_val;
  un_val.len = 1;
  un_val.keys = lmalloc (&alloc, sizeof (string));
  un_val.keys[0] = (string){ .len = 4, .data = "opt1" };
  un_val.types = lmalloc (&alloc, sizeof (type));
  un_val.types[0] = union_inner;

  type union_type = { .type = T_UNION, .un = un_val };

  // Enum with two variants
  enum_t en_val;
  en_val.len = 2;
  en_val.keys = lmalloc (&alloc, sizeof (string) * 2);
  en_val.keys[0] = (string){ .len = 5, .data = "first" };
  en_val.keys[1] = (string){ .len = 6, .data = "second" };

  type enum_type = { .type = T_ENUM, .en = en_val };

  // Varray of a primitive
  type varray_inner = { .type = T_PRIM, .p = F64 };
  varray_t va_val;
  va_val.rank = 1;
  va_val.t = &varray_inner;

  type varray_type = { .type = T_VARRAY, .va = va_val };

  // Sarray of a primitive
  type sarray_inner = { .type = T_PRIM, .p = I32 };
  sarray_t sa_val;
  sa_val.rank = 2;
  sa_val.dims = lmalloc (&alloc, sizeof (u16) * 2);
  sa_val.dims[0] = 10;
  sa_val.dims[1] = 20;
  sa_val.t = &sarray_inner;

  type sarray_type = { .type = T_SARRAY, .sa = sa_val };

  // --- Wrap all of these inside a top-level struct ---

  struct_t top_st_val;
  top_st_val.len = 6;
  top_st_val.keys = lmalloc (&alloc, sizeof (string) * 6);
  top_st_val.keys[0] = (string){ .len = 4, .data = "prim" };
  top_st_val.keys[1] = (string){ .len = 6, .data = "struct" };
  top_st_val.keys[2] = (string){ .len = 5, .data = "union" };
  top_st_val.keys[3] = (string){ .len = 4, .data = "enum" };
  top_st_val.keys[4] = (string){ .len = 6, .data = "varray" };
  top_st_val.keys[5] = (string){ .len = 6, .data = "sarray" };

  top_st_val.types = lmalloc (&alloc, sizeof (type) * 6);
  top_st_val.types[0] = prim;
  top_st_val.types[1] = struct_type;
  top_st_val.types[2] = union_type;
  top_st_val.types[3] = enum_type;
  top_st_val.types[4] = varray_type;
  top_st_val.types[5] = sarray_type;

  type top;
  top.type = T_STRUCT;
  top.st = top_st_val;

  // --- Now actually test serialize/deserialize ---

  u16 size = 0;
  err_t r = type_get_serial_size (&size, &top);
  test_assert_int_equal (r, SUCCESS);

  u8 *buf = malloc (size);
  test_assert_int_equal (buf != NULL, 1);

  r = type_serialize ((type_serialize_params){
      .src = &top,
      .dest = buf,
      .dlen = size });
  test_assert_int_equal (r, SUCCESS);

  type roundtrip;
  i_memset (&roundtrip, 0, sizeof (roundtrip));

  r = type_deserialize ((type_deserialize_params){
      .dest = &roundtrip,
      .type_allocator = &alloc,
      .src = buf,
      .dlen = size });
  test_assert_int_equal (r, SUCCESS);

  // --- spot check the deserialized data ---

  test_assert_int_equal (roundtrip.type, T_STRUCT);
  test_assert_int_equal (roundtrip.st.len, 6);
  test_assert_int_equal (roundtrip.st.keys[0].len, 4);
  test_assert_int_equal (i_memcmp (roundtrip.st.keys[0].data, "prim", 4), 0);

  test_assert_int_equal (roundtrip.st.types[0].type, T_PRIM);
  test_assert_int_equal (roundtrip.st.types[0].p, U16);

  test_assert_int_equal (roundtrip.st.types[1].type, T_STRUCT);
  test_assert_int_equal (roundtrip.st.types[2].type, T_UNION);
  test_assert_int_equal (roundtrip.st.types[3].type, T_ENUM);
  test_assert_int_equal (roundtrip.st.types[4].type, T_VARRAY);
  test_assert_int_equal (roundtrip.st.types[5].type, T_SARRAY);

  free (buf);
}
