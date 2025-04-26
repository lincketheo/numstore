#include "typing.h"
#include "dev/errors.h"
#include "dev/testing.h"
#include "intf/mm.h"
#include "intf/types.h"
#include "utils/bounds.h"

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

err_t
struct_t_bits_size (u64 *dest, struct_t *t)
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

//////////////////////////////// Strict Array

err_t
sarray_t_bits_size (u64 *dest, sarray_t *sa)
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
        return struct_t_bits_size (dest, t->st);
      }
    case T_SARRAY:
      {
        return sarray_t_bits_size (dest, t->sa);
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
        if ((ret = ser_size_add (&size, 1)))
          {
            return ret;
          }
        break;
      }

    case T_STRUCT:
      {
        if ((ret = ser_size_struct (&size, t->st)))
          {
            return ret;
          }
        break;
      }

    case T_UNION:
      {
        if ((ret = ser_size_union (&size, t->un)))
          {
            return ret;
          }
        break;
      }

    case T_ENUM:
      {
        if ((ret = ser_size_enum (&size, t->en)))
          {
            return ret;
          }
        break;
      }

    case T_VARRAY:
      {
        if ((ret = ser_size_varray (&size, t->va)))
          {
            return ret;
          }
        break;
      }

    case T_SARRAY:
      {
        if ((ret = ser_size_sarray (&size, t->sa)))
          {
            return ret;
          }
        break;
      }

    default:
      {
        ASSERT (0);
        break;
      }
    }

  *dest = size;
  return SUCCESS;
}

//////////////////////////////// Serialize

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
ser_write_u32 (u8 *dest, u16 dlen, u16 *idx, u32 val)
{
  return ser_write_bytes (dest, dlen, idx, &val, sizeof (val));
}

static inline err_t
type_serialize_prim (type_serialize_params p, u16 *idx)
{
  // write the subtype byte
  return ser_write_u8 (p.dest, p.dlen, idx, p.src->p);
}

static inline err_t
type_serialize_struct (type_serialize_params p, u16 *idx)
{
  err_t ret;
  struct_t *st = p.src->st;

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
        u16 sublen;
        if ((ret = type_get_serial_size (&sublen, &st->types[i])))
          {
            return ret;
          }
        type_serialize_params subp = {
          .src = &st->types[i],
          .dest = p.dest + *idx,
          .dlen = (u16)(p.dlen - *idx)
        };
        if ((ret = type_serialize (subp)))
          {
            return ret;
          }
        *idx += sublen;
      }
    }

  return SUCCESS;
}

static inline err_t
type_serialize_union (type_serialize_params p, u16 *idx)
{
  err_t ret;
  union_t *un = p.src->un;

  // number of variants
  if ((ret = ser_write_u32 (p.dest, p.dlen, idx, un->len)))
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
        u16 sublen;
        if ((ret = type_get_serial_size (&sublen, &un->types[i])))
          {
            return ret;
          }
        type_serialize_params subp = {
          .src = &un->types[i],
          .dest = p.dest + *idx,
          .dlen = (u16)(p.dlen - *idx)
        };
        if ((ret = type_serialize (subp)))
          {
            return ret;
          }
        *idx += sublen;
      }
    }

  return SUCCESS;
}

static inline err_t
type_serialize_enum (type_serialize_params p, u16 *idx)
{
  err_t ret;
  enum_t *en = p.src->en;

  // number of entries
  if ((ret = ser_write_u32 (p.dest, p.dlen, idx, en->len)))
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

static inline err_t
type_serialize_varray (type_serialize_params p, u16 *idx)
{
  err_t ret;
  varray_t *va = p.src->va;

  // rank
  if ((ret = ser_write_u16 (p.dest, p.dlen, idx, va->rank)))
    {
      return ret;
    }

  // nested element type
  {
    u16 sublen;
    if ((ret = type_get_serial_size (&sublen, va->t)))
      {
        return ret;
      }
    type_serialize_params subp = {
      .src = va->t,
      .dest = p.dest + *idx,
      .dlen = (u16)(p.dlen - *idx)
    };
    if ((ret = type_serialize (subp)))
      {
        return ret;
      }
    *idx += sublen;
  }

  return SUCCESS;
}

static inline err_t
type_serialize_sarray (type_serialize_params p, u16 *idx)
{
  err_t ret;
  sarray_t *sa = p.src->sa;

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
    u16 sublen;
    if ((ret = type_get_serial_size (&sublen, sa->t)))
      {
        return ret;
      }
    type_serialize_params subp = {
      .src = sa->t,
      .dest = p.dest + *idx,
      .dlen = (u16)(p.dlen - *idx)
    };
    if ((ret = type_serialize (subp)))
      {
        return ret;
      }
    *idx += sublen;
  }

  return SUCCESS;
}

err_t
type_serialize (type_serialize_params params)
{
  type_assert (params.src);
  ASSERT (params.dest);

  u16 idx = 0;
  err_t ret;

  // write the type tag
  if ((ret = ser_write_u8 (
           params.dest, params.dlen, &idx,
           (u8)params.src->type)))
    {
      return ret;
    }

  switch (params.src->type)
    {
    case T_PRIM:
      {
        return type_serialize_prim (params, &idx);
      }
    case T_STRUCT:
      {
        return type_serialize_struct (params, &idx);
      }
    case T_UNION:
      {
        return type_serialize_union (params, &idx);
      }
    case T_ENUM:
      {
        return type_serialize_enum (params, &idx);
      }
    case T_VARRAY:
      {
        return type_serialize_varray (params, &idx);
      }
    case T_SARRAY:
      {
        return type_serialize_sarray (params, &idx);
      }
    default:
      {
        ASSERT (0);
        return ERR_INVALID_STATE;
      }
    }
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
ser_read_u32 (const u8 *src, u16 dlen, u16 *idx, u32 *out)
{
  return ser_read_bytes (src, dlen, idx, out, sizeof (*out));
}

static inline err_t
type_deserialize_prim (type_deserialize_params p, u16 *idx)
{
  u8 _p = (u8)p.dest->p;
  return ser_read_u8 (p.src, p.dlen, idx, &_p);
}

static inline err_t
type_deserialize_struct (type_deserialize_params p, u16 *idx)
{
  err_t ret;
  struct_t *st;
  u32 len;

  // allocate struct descriptor
  st = lmalloc (p.type_allocator, sizeof (*st));
  if (st == NULL)
    {
      return ERR_NOMEM;
    }
  p.dest->st = st;

  // read number of fields
  if ((ret = ser_read_u32 (p.src, p.dlen, idx, &len)))
    {
      return ret;
    }
  st->len = len;

  // allocate arrays
  st->keys = lmalloc (p.type_allocator, len * sizeof (*st->keys));
  if (st->keys == NULL)
    {
      return ERR_NOMEM;
    }
  st->types = lmalloc (p.type_allocator, len * sizeof (*st->types));
  if (st->types == NULL)
    {
      return ERR_NOMEM;
    }

  // for each entry
  for (u32 i = 0; i < len; ++i)
    {
      // read key length
      if ((ret = ser_read_u16 (p.src, p.dlen, idx, &st->keys[i].len)))
        {
          return ret;
        }
      // allocate and read key data
      st->keys[i].data = lmalloc (
          p.type_allocator,
          st->keys[i].len);
      if (st->keys[i].data == NULL)
        {
          return ERR_NOMEM;
        }
      if ((ret = ser_read_bytes (
               p.src, p.dlen, idx,
               st->keys[i].data,
               st->keys[i].len)))
        {
          return ret;
        }

      // nested type
      {
        u16 sublen;
        if ((ret = type_get_serial_size (&sublen, &st->types[i])))
          {
            return ret;
          }
        type_deserialize_params subp = {
          .dest = &st->types[i],
          .type_allocator = p.type_allocator,
          .src = p.src + *idx,
          .dlen = (u16)(p.dlen - *idx)
        };
        if ((ret = type_deserialize (subp)))
          {
            return ret;
          }
        *idx += sublen;
      }
    }

  return SUCCESS;
}

static inline err_t
type_deserialize_union (type_deserialize_params p, u16 *idx)
{
  err_t ret;
  union_t *un;
  u32 len;

  un = lmalloc (p.type_allocator, sizeof (*un));
  if (un == NULL)
    {
      return ERR_NOMEM;
    }
  p.dest->un = un;

  // read variant count
  if ((ret = ser_read_u32 (p.src, p.dlen, idx, &len)))
    {
      return ret;
    }
  un->len = len;

  un->keys = lmalloc (p.type_allocator, len * sizeof (*un->keys));
  if (un->keys == NULL)
    {
      return ERR_NOMEM;
    }
  un->types = lmalloc (p.type_allocator, len * sizeof (*un->types));
  if (un->types == NULL)
    {
      return ERR_NOMEM;
    }

  for (u32 i = 0; i < len; ++i)
    {
      // key length
      if ((ret = ser_read_u16 (p.src, p.dlen, idx, &un->keys[i].len)))
        {
          return ret;
        }
      // allocate & read key data
      un->keys[i].data = lmalloc (
          p.type_allocator,
          un->keys[i].len);
      if (un->keys[i].data == NULL)
        {
          return ERR_NOMEM;
        }
      if ((ret = ser_read_bytes (
               p.src, p.dlen, idx,
               un->keys[i].data,
               un->keys[i].len)))
        {
          return ret;
        }

      // nested type
      {
        u16 sublen;
        if ((ret = type_get_serial_size (&sublen, &un->types[i])))
          {
            return ret;
          }
        type_deserialize_params subp = {
          .dest = &un->types[i],
          .type_allocator = p.type_allocator,
          .src = p.src + *idx,
          .dlen = (u16)(p.dlen - *idx)
        };
        if ((ret = type_deserialize (subp)))
          {
            return ret;
          }
        *idx += sublen;
      }
    }

  return SUCCESS;
}

static inline err_t
type_deserialize_enum (type_deserialize_params p, u16 *idx)
{
  err_t ret;
  enum_t *en;
  u32 len;

  en = lmalloc (p.type_allocator, sizeof (*en));
  if (en == NULL)
    {
      return ERR_NOMEM;
    }
  p.dest->en = en;

  // read entry count
  if ((ret = ser_read_u32 (p.src, p.dlen, idx, &len)))
    {
      return ret;
    }
  en->len = len;

  en->keys = lmalloc (p.type_allocator, len * sizeof (*en->keys));
  if (en->keys == NULL)
    {
      return ERR_NOMEM;
    }

  for (u32 i = 0; i < len; ++i)
    {
      // key length
      if ((ret = ser_read_u16 (p.src, p.dlen, idx, &en->keys[i].len)))
        {
          return ret;
        }
      // allocate & read key data
      en->keys[i].data = lmalloc (
          p.type_allocator,
          en->keys[i].len);
      if (en->keys[i].data == NULL)
        {
          return ERR_NOMEM;
        }
      if ((ret = ser_read_bytes (
               p.src, p.dlen, idx,
               en->keys[i].data,
               en->keys[i].len)))
        {
          return ret;
        }
    }

  return SUCCESS;
}

static inline err_t
type_deserialize_varray (type_deserialize_params p, u16 *idx)
{
  err_t ret;
  varray_t *va;
  u16 rank;

  va = lmalloc (p.type_allocator, sizeof (*va));
  if (va == NULL)
    {
      return ERR_NOMEM;
    }
  p.dest->va = va;

  // read rank
  if ((ret = ser_read_u16 (p.src, p.dlen, idx, &rank)))
    {
      return ret;
    }
  va->rank = rank;

  // nested type
  {
    u16 sublen;
    if ((ret = type_get_serial_size (&sublen, va->t)))
      {
        return ret;
      }
    type_deserialize_params subp = {
      .dest = va->t,
      .type_allocator = p.type_allocator,
      .src = p.src + *idx,
      .dlen = (u16)(p.dlen - *idx)
    };
    if ((ret = type_deserialize (subp)))
      {
        return ret;
      }
    *idx += sublen;
  }

  return SUCCESS;
}

static inline err_t
type_deserialize_sarray (type_deserialize_params p, u16 *idx)
{
  err_t ret;
  sarray_t *sa;
  u16 rank;

  sa = lmalloc (p.type_allocator, sizeof (*sa));
  if (sa == NULL)
    {
      return ERR_NOMEM;
    }
  p.dest->sa = sa;

  // read rank
  if ((ret = ser_read_u16 (p.src, p.dlen, idx, &rank)))
    {
      return ret;
    }
  sa->rank = rank;

  // fixed dims
  {
    u16 dims_size = sa->rank * (u16)sizeof (*sa->dims);
    if (!can_mul_u16 ((u16)sizeof (*sa->dims), sa->rank))
      {
        return ERR_ARITH;
      }
    sa->dims = lmalloc (p.type_allocator, dims_size);
    if (sa->dims == NULL)
      {
        return ERR_NOMEM;
      }
    if ((ret = ser_read_bytes (
             p.src, p.dlen, idx,
             sa->dims,
             dims_size)))
      {
        return ret;
      }
  }

  // nested type
  {
    u16 sublen;
    if ((ret = type_get_serial_size (&sublen, sa->t)))
      {
        return ret;
      }
    type_deserialize_params subp = {
      .dest = sa->t,
      .type_allocator = p.type_allocator,
      .src = p.src + *idx,
      .dlen = (u16)(p.dlen - *idx)
    };
    if ((ret = type_deserialize (subp)))
      {
        return ret;
      }
    *idx += sublen;
  }

  return SUCCESS;
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
        return type_deserialize_prim (params, &idx);
      }
    case T_STRUCT:
      {
        return type_deserialize_struct (params, &idx);
      }
    case T_UNION:
      {
        return type_deserialize_union (params, &idx);
      }
    case T_ENUM:
      {
        return type_deserialize_enum (params, &idx);
      }
    case T_VARRAY:
      {
        return type_deserialize_varray (params, &idx);
      }
    case T_SARRAY:
      {
        return type_deserialize_sarray (params, &idx);
      }
    default:
      {
        return ERR_INVALID_STATE;
      }
    }
}

TEST (type_serialize_deserialize)
{
  lalloc alloc;
  lalloc_create (&alloc, 4096);

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

  type struct_type = { .type = T_STRUCT, .st = &st_val };

  // Union with one variant
  type union_inner = { .type = T_PRIM, .p = U32 };
  union_t un_val;
  un_val.len = 1;
  un_val.keys = lmalloc (&alloc, sizeof (string));
  un_val.keys[0] = (string){ .len = 4, .data = "opt1" };
  un_val.types = lmalloc (&alloc, sizeof (type));
  un_val.types[0] = union_inner;

  type union_type = { .type = T_UNION, .un = &un_val };

  // Enum with two variants
  enum_t en_val;
  en_val.len = 2;
  en_val.keys = lmalloc (&alloc, sizeof (string) * 2);
  en_val.keys[0] = (string){ .len = 5, .data = "first" };
  en_val.keys[1] = (string){ .len = 6, .data = "second" };

  type enum_type = { .type = T_ENUM, .en = &en_val };

  // Varray of a primitive
  type varray_inner = { .type = T_PRIM, .p = F64 };
  varray_t va_val;
  va_val.rank = 1;
  va_val.t = &varray_inner;

  type varray_type = { .type = T_VARRAY, .va = &va_val };

  // Sarray of a primitive
  type sarray_inner = { .type = T_PRIM, .p = I32 };
  sarray_t sa_val;
  sa_val.rank = 2;
  sa_val.dims = lmalloc (&alloc, sizeof (u16) * 2);
  sa_val.dims[0] = 10;
  sa_val.dims[1] = 20;
  sa_val.t = &sarray_inner;

  type sarray_type = { .type = T_SARRAY, .sa = &sa_val };

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
  top.st = &top_st_val;

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
  test_assert_int_equal (roundtrip.st->len, 6);
  test_assert_int_equal (roundtrip.st->keys[0].len, 4);
  test_assert_int_equal (i_memcmp (roundtrip.st->keys[0].data, "prim", 4), 0);

  test_assert_int_equal (roundtrip.st->types[0].type, T_PRIM);
  test_assert_int_equal (roundtrip.st->types[0].p, U16);

  test_assert_int_equal (roundtrip.st->types[1].type, T_STRUCT);
  test_assert_int_equal (roundtrip.st->types[2].type, T_UNION);
  test_assert_int_equal (roundtrip.st->types[3].type, T_ENUM);
  test_assert_int_equal (roundtrip.st->types[4].type, T_VARRAY);
  test_assert_int_equal (roundtrip.st->types[5].type, T_SARRAY);

  free (buf);
}
