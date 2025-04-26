#pragma once

/**
 * A cbuffer is a circular buffer.
 */
#include "intf/types.h"

typedef struct
{
  u8 *data;
  u32 cap;
  u32 head;
  u32 tail;
  bool isfull;
} cbuffer;

cbuffer cbuffer_create (u8 *data, u32 cap);
bool cbuffer_isempty (const cbuffer *b);
u32 cbuffer_len (const cbuffer *b);
u32 cbuffer_avail (const cbuffer *b);
u32 cbuffer_read (void *dest, u32 size, u32 n, cbuffer *b);
u32 cbuffer_copy (void *dest, u32 size, u32 n, const cbuffer *b);
u32 cbuffer_write (const void *src, u32 size, u32 n, cbuffer *b);
bool cbuffer_get (u8 *dest, const cbuffer *b, int idx);
bool cbuffer_peek_dequeue (u8 *dest, const cbuffer *b);
bool cbuffer_enqueue (cbuffer *b, u8 val);
bool cbuffer_dequeue (u8 *dest, cbuffer *b);
