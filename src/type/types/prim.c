#include "type/types/prim.h"

#include "dev/assert.h"  // DEFINE_DBG_ASSERT_I
#include "intf/stdlib.h" // i_snprintf
#include "type/types.h"  // u32-u64

DEFINE_DBG_ASSERT_I (prim_t, prim_t, s)
{
  ASSERT (s);
  error e = error_create (NULL);
  ASSERT (prim_t_validate (s, &e) == SUCCESS);
}

err_t
prim_t_validate (const prim_t *t, error *e)
{
  ASSERT (t);
  if (!(*t <= CU128 && *t >= U8))
    {
      return error_causef (
          e, ERR_INVALID_TYPE,
          "Prim: Invalid numerical prim value: %d. "
          "Expect to be in range %d, %d",
          *t, U8, CU128);
    }

  return SUCCESS;
}

const char *
prim_to_str (prim_t p)
{
  prim_t_assert (&p);
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
    }
  UNREACHABLE ();
  return "";
}

int
prim_t_snprintf (char *str, u32 size, const prim_t *p)
{
  prim_t_assert (p);

  char *out = str;
  u32 avail = size;
  int len = 0;
  int n;
  const char *name = prim_to_str (*p);

  n = i_snprintf (out, avail, "%s", name);
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

  return len;
}

u32
prim_t_byte_size (const prim_t *t)
{
  prim_t_assert (t);

  switch (*t)
    {
    case U8:
    case I8:
      return 1;

    case U16:
    case I16:
    case F16:
    case CI16:
    case CU16:
      return 2;

    case U32:
    case I32:
    case F32:
    case CF32:
    case CI32:
    case CU32:
      return 4;

    case U64:
    case I64:
    case F64:
    case CF64:
    case CI64:
    case CU64:
      return 8;

    case F128:
    case CF128:
    case CI128:
    case CU128:
      return 16;

    case CF256:
      return 32;
    }

  UNREACHABLE ();
  return 0;
}

void
prim_t_serialize (serializer *dest, const prim_t *src)
{
  prim_t_assert (src);
  bool ret;

  /**
   * PRIM
   */
  ret = srlizr_write_u8 (dest, (u8)*src);
  ASSERT (ret);
}

err_t
prim_t_deserialize (prim_t *dest, deserializer *src, error *e)
{
  ASSERT (dest);

  u8 p;
  bool ret = dsrlizr_read_u8 (&p, src);
  if (!ret)
    {
      ret = error_causef (
          e, ERR_TYPE_DESER,
          "Prim Deserialize. Expected a length header");
    }

  prim_t _p = p;

  err_t_wrap (prim_t_validate (&_p, e), e);

  prim_t_assert (&_p);

  *dest = _p;

  return SUCCESS;
}
