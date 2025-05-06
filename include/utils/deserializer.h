#pragma once

#include "intf/types.h"

typedef struct
{
  const u8 *data;
  u32 head;
  const u32 dlen;
} deserializer;

deserializer dsrlizr_create (u8 *data, u32 dlen);

bool dsrlizr_read (u8 *dest, u32 dlen, deserializer *src);

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
static inline bool
dsrlizr_read_u8 (u8 *d, deserializer *src)
{
  return dsrlizr_read ((u8 *)d, sizeof *d, src);
}
static inline bool
dsrlizr_read_u16 (u16 *d, deserializer *src)
{
  return dsrlizr_read ((u8 *)d, sizeof *d, src);
}
static inline bool
dsrlizr_read_u32 (u32 *d, deserializer *src)
{
  return dsrlizr_read ((u8 *)d, sizeof *d, src);
}
static inline bool
dsrlizr_read_u64 (u64 *d, deserializer *src)
{
  return dsrlizr_read ((u8 *)d, sizeof *d, src);
}
#pragma GCC diagnostic pop
