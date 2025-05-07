#include "type/types.h"
#include "dev/assert.h"
#include "dev/errors.h"
#include "intf/mm.h"

DEFINE_DBG_ASSERT_I (type, unchecked_type, t)
{
  ASSERT (t);
}

DEFINE_DBG_ASSERT_I (type, valid_type, t)
{
  ASSERT (type_is_valid (t));
}

bool
type_is_valid (const type *t)
{
  unchecked_type_assert (t);
  switch (t->type)
    {
    case T_PRIM:
      {
        return prim_t_is_valid (&t->p);
      }
    case T_STRUCT:
      {
        return struct_t_is_valid (&t->st);
      }
    case T_UNION:
      {
        return union_t_is_valid (&t->un);
      }
    case T_ENUM:
      {
        return enum_t_is_valid (&t->en);
      }
    case T_SARRAY:
      {
        return sarray_t_is_valid (&t->sa);
      }
    default:
      {
        ASSERT (0);
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
        ASSERT (0);
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
        ASSERT (0);
        return 0;
      }
    }
}

/**
void
type_free_internals_forgiving (type *t, lalloc *alloc)
{
  valid_type_assert (t);

  switch (t->type)
    {
    case T_PRIM:
      {
        break;
      }
    case T_STRUCT:
      {
        struct_t_free_internals_forgiving (&t->st, alloc);
        break;
      }
    case T_UNION:
      {
        union_t_free_internals_forgiving (&t->un, alloc);
        break;
      }
    case T_ENUM:
      {
        enum_t_free_internals_forgiving (&t->en, alloc);
        break;
      }
    case T_SARRAY:
      {
        sarray_t_free_internals_forgiving (&t->sa, alloc);
        break;
      }
    default:
      {
        ASSERT (0);
        break;
      }
    }
}

void
type_free_internals (type *t, lalloc *alloc)
{
  valid_type_assert (t);

  switch (t->type)
    {
    case T_PRIM:
      {
        break;
      }
    case T_STRUCT:
      {
        struct_t_free_internals (&t->st, alloc);
        break;
      }
    case T_UNION:
      {
        union_t_free_internals (&t->un, alloc);
        break;
      }
    case T_ENUM:
      {
        enum_t_free_internals (&t->en, alloc);
        break;
      }
    case T_SARRAY:
      {
        sarray_t_free_internals (&t->sa, alloc);
        break;
      }
    default:
      {
        ASSERT (0);
        break;
      }
    }
}
*/

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
        ASSERT (0);
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
        ASSERT (0);
        break;
      }
    }
}

err_t
type_deserialize (alloced_type *dest, deserializer *src)
{
  u32 dcap = 10;

  /**
   * TODO - get rid of the while loop
   */
  u8 *data = lmalloc (dest->alloc, dcap);
  if (!data)
    {
      return ERR_NOMEM;
    }

  while (true)
    {
      salloc alloc;
      salloc_create_from (&alloc, data, dcap);
      err_t ret;

      switch (dest->ret.type)
        {
        case T_PRIM:
          {
            ret = prim_t_deserialize (&dest->ret.p, src);
            break;
          }
        case T_STRUCT:
          {
            ret = struct_t_deserialize (&dest->ret.st, src, &alloc);
            break;
          }
        case T_UNION:
          {
            ret = union_t_deserialize (&dest->ret.un, src, &alloc);
            break;
          }
        case T_ENUM:
          {
            ret = enum_t_deserialize (&dest->ret.en, src, &alloc);
            break;
          }
        case T_SARRAY:
          {
            ret = sarray_t_deserialize (&dest->ret.sa, src, &alloc);
            break;
          }
        default:
          {
            ASSERT (0);
            return ERR_FALLBACK;
          }

          if (ret == SUCCESS)
            {
              // TODO - it would be nice to clip data, but
              // we would need to remap all the pointers
              dest->data = data;
              dest->dlen = alloc.len;
              return ret;
            }

          /**
           * Increase the size and try again
           */
          u8 *data = lrealloc (dest->alloc, data, dcap);
          if (!data)
            {
              return ERR_NOMEM;
            }
        }
    }
}
