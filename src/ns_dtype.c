#include "ns_dtype.h"
#include "ns_errors.h"

#define S8                                                                    \
  U8:                                                                         \
  case I8

#define S16                                                                   \
  U16:                                                                        \
  case I16:                                                                   \
  case F16:                                                                   \
  case CI16:                                                                  \
  case CU16

#define S32                                                                   \
  U32:                                                                        \
  case I32:                                                                   \
  case CF32:                                                                  \
  case CI32:                                                                  \
  case CU32:                                                                  \
  case F32

#define S64                                                                   \
  U64:                                                                        \
  case I64:                                                                   \
  case CF64:                                                                  \
  case CI64:                                                                  \
  case CU64:                                                                  \
  case F64

#define S128                                                                  \
  CF128:                                                                      \
  case CI128:                                                                 \
  case CU128

ns_size
dtype_sizeof (dtype d)
{
  switch (d)
    {
    case S8:
      return 1;
    case S16:
      return 2;
    case S32:
      return 4;
    case S64:
      return 8;
    case S128:
      return 16;
    default:
      unreachable ();
    }
}

ns_ret_t
inttodtype (dtype *dest, int type)
{
  ASSERT (dest);
  if (type < 0)
    return -1;
  if (type > 16)
    return -1;
  *dest = type;
  return 0;
}
