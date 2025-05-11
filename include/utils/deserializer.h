#pragma once

#include "ds/strings.h"
#include "intf/stdlib.h"
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
static inline const char *
dsrlizr_read_string (u16 *len, deserializer *src)
{
  u32 start = src->head; // Save state before

  /**
   * Read string length first
   */
  u16 _len;
  if (dsrlizr_read_u16 (&_len, src))
    {
      goto failed;
    }

  const char *ret = (const char *)(src->data + src->head);

  /**
   * Then skip string length
   */
  if (dsrlizr_read (NULL, _len, src))
    {
      goto failed;
    }

  *len = _len;
  return ret;

failed:
  src->head = start;
  return NULL;
}
#pragma GCC diagnostic pop
