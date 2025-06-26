#include "numstore/type/types.h"

#include "core/dev/assert.h"   // TODO
#include "core/errors/error.h" // TODO
#include "core/intf/io.h"      // TODO
#include "core/intf/logging.h" // TODO
#include "core/math/random.h"  // TODO

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

i32
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
  dest->type = (type_t)type;
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
            e, ERR_INVALID_ARGUMENT,
            "Unknown type code: %d", ret);
      }
    }
}

err_t
type_random (type *dest, lalloc *alloc, u32 depth, error *e)
{
  ASSERT (dest);

  if (depth == 0)
    {
      dest->type = T_PRIM;
      dest->p = prim_t_random ();
      return SUCCESS;
    }

  static const type_t weighted[] = {
    T_PRIM, T_PRIM, T_PRIM, T_PRIM,
    T_ENUM, T_STRUCT, T_UNION, T_SARRAY
  };

  dest->type = weighted[randu32 (0, sizeof (weighted) / sizeof (weighted[0]))];

  switch (dest->type)
    {
    case T_PRIM:
      {
        dest->p = prim_t_random ();
        return SUCCESS;
      }

    case T_ENUM:
      {
        return enum_t_random (&dest->en, alloc, e);
      }

    case T_STRUCT:
      {
        return struct_t_random (&dest->st, alloc, depth - 1, e);
      }

    case T_UNION:
      {
        return union_t_random (&dest->un, alloc, depth - 1, e);
      }

    case T_SARRAY:
      {
        return sarray_t_random (&dest->sa, alloc, depth - 1, e);
      }

    default:
      {
        return error_causef (e, ERR_NOMEM, "Invalid type tag");
      }
    }
}

bool
type_equal (const type *left, const type *right)
{
  if (left->type != right->type)
    {
      return false;
    }

  switch (left->type)
    {
    case T_PRIM:
      {
        return left->p == right->p;
      }
    case T_STRUCT:
      {
        return struct_t_equal (&left->st, &right->st);
      }
    case T_UNION:
      {
        return union_t_equal (&left->un, &right->un);
      }
    case T_ENUM:
      {
        return enum_t_equal (&left->en, &right->en);
      }
    case T_SARRAY:
      {
        return sarray_t_equal (&left->sa, &right->sa);
      }
    default:
      {
        UNREACHABLE ();
      }
    }
}

err_t
i_log_type (type t, error *e)
{
  i32 len = type_snprintf (NULL, 0, &t);
  if (len < 0)
    {
      return error_causef (e, ERR_IO, "Failed to call snprintf\n");
    }

  char *dest = i_malloc (len + 1, sizeof *dest);
  if (dest == NULL)
    {
      return error_causef (e, ERR_NOMEM, "Failed to allocate dest for log type\n");
    }

  len = type_snprintf (dest, len + 1, &t);
  if (len < 0)
    {
      i_free (dest);
      return error_causef (e, ERR_IO, "Failed to call snprintf\n");
    }

  i_log_info ("%.*s\n", len, dest);
  i_free (dest);

  return SUCCESS;
}
