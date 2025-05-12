#include "type/types.h"
#include "dev/assert.h"
#include "errors/error.h"
#include "mm/lalloc.h"
#include "type/types/enum.h"
#include "type/types/prim.h"
#include "type/types/union.h"
#include "utils/deserializer.h"

DEFINE_DBG_ASSERT_I (type, unchecked_type, t)
{
  ASSERT (t);
}

DEFINE_DBG_ASSERT_I (type, valid_type, t)
{
  error e = error_create (NULL);
  ASSERT (type_validate (t, &e) == SUCCESS);
}

err_t
type_validate (const type *t, error *e)
{
  unchecked_type_assert (t);
  switch (t->type)
    {
    case T_PRIM:
      {
        return prim_t_validate (&t->p, e);
      }
    case T_STRUCT:
      {
        return struct_t_validate (&t->st, e);
      }
    case T_UNION:
      {
        return union_t_validate (&t->un, e);
      }
    case T_ENUM:
      {
        return enum_t_validate (&t->en, e);
      }
    case T_SARRAY:
      {
        return sarray_t_validate (&t->sa, e);
      }
    default:
      {
        UNREACHABLE ();
        return -1;
      }
    }
}

int
type_snprintf (char *str, u32 size, type *t)
{
  valid_type_assert (t);

  switch (t->type)
    {
    case T_PRIM:
      {
        return prim_t_snprintf (str, size, &t->p);
      }
    case T_STRUCT:
      {
        return struct_t_snprintf (str, size, &t->st);
      }
    case T_UNION:
      {
        return union_t_snprintf (str, size, &t->un);
      }
    case T_ENUM:
      {
        return enum_t_snprintf (str, size, &t->en);
      }
    case T_SARRAY:
      {
        return sarray_t_snprintf (str, size, &t->sa);
      }
    default:
      {
        UNREACHABLE ();
        return -1;
      }
    }
}

u32
type_byte_size (const type *t)
{
  valid_type_assert (t);

  switch (t->type)
    {
    case T_PRIM:
      {
        return prim_t_byte_size (&t->p);
      }

    case T_STRUCT:
      {
        return struct_t_byte_size (&t->st);
      }

    case T_UNION:
      {
        return union_t_byte_size (&t->un);
      }

    case T_ENUM:
      {
        return enum_t_byte_size (&t->en);
      }

    case T_SARRAY:
      {
        return sarray_t_byte_size (&t->sa);
      }

    default:
      {
        UNREACHABLE ();
        return 0;
      }
    }
}

u32
type_get_serial_size (const type *t)
{
  valid_type_assert (t);

  /**
   * LABEL TYPE
   */
  u32 ret = sizeof (u8);

  switch (t->type)
    {
    case T_PRIM:
      {
        return ret + sizeof (u8);
      }
    case T_STRUCT:
      {
        return ret + struct_t_get_serial_size (&t->st);
      }
    case T_UNION:
      {
        return ret + union_t_get_serial_size (&t->un);
      }
    case T_ENUM:
      {
        return ret + enum_t_get_serial_size (&t->en);
      }
    case T_SARRAY:
      {
        return ret + sarray_t_get_serial_size (&t->sa);
      }
    default:
      {
        UNREACHABLE ();
        return 0;
      }
    }
}

void
type_serialize (serializer *dest, const type *src)
{
  valid_type_assert (src);
  bool ret;

  ret = srlizr_write_u8 (dest, (u8)src->type);
  ASSERT (ret);

  switch (src->type)
    {
    case T_PRIM:
      {
        prim_t_serialize (dest, &src->p);
        break;
      }
    case T_STRUCT:
      {
        struct_t_serialize (dest, &src->st);
        break;
      }
    case T_UNION:
      {
        union_t_serialize (dest, &src->un);
        break;
      }
    case T_ENUM:
      {
        enum_t_serialize (dest, &src->en);
        break;
      }
    case T_SARRAY:
      {
        sarray_t_serialize (dest, &src->sa);
        break;
      }
    default:
      {
        UNREACHABLE ();
        break;
      }
    }
}

err_t
type_deserialize (type *dest, deserializer *src, lalloc *alloc, error *e)
{
  u8 type;
  bool ret = dsrlizr_read_u8 (&type, src);
  switch (type)
    {
    case T_PRIM:
      {
        return prim_t_deserialize (&dest->p, src, e);
      }
    case T_STRUCT:
      {
        return struct_t_deserialize (&dest->st, src, alloc, e);
      }
    case T_UNION:
      {
        return union_t_deserialize (&dest->un, src, alloc, e);
      }
    case T_ENUM:
      {
        return enum_t_deserialize (&dest->en, src, alloc, e);
      }
    case T_SARRAY:
      {
        return sarray_t_deserialize (&dest->sa, src, alloc, e);
      }
    default:
      {
        return error_causef (
            e, ERR_INVALID_TYPE,
            "Unknown type code: %d", ret);
      }
    }
}
