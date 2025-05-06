#pragma once

#include "intf/types.h"

typedef struct
{
  u8 *data;
  u32 dlen;
  const u32 dcap;
} serializer;

serializer srlizr_create (u8 *data, u32 dcap);

bool srlizr_write (serializer *dest, const u8 *src, u32 len);

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
static inline bool
srlizr_write_u8 (serializer *dest, u8 d)
{
  return srlizr_write (dest, (u8 *)&d, sizeof (d));
}
static inline bool
srlizr_write_u16 (serializer *dest, u16 d)
{
  return srlizr_write (dest, (u8 *)&d, sizeof (d));
}
static inline bool
srlizr_write_u32 (serializer *dest, u32 d)
{
  return srlizr_write (dest, (u8 *)&d, sizeof (d));
}
static inline bool
srlizr_write_u64 (serializer *dest, u64 d)
{
  return srlizr_write (dest, (u8 *)&d, sizeof (d));
}
#pragma GCC diagnostic pop
