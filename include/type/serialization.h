#pragma once

#include "dev/errors.h"
#include "intf/types.h"
#include "utils/bounds.h"

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
