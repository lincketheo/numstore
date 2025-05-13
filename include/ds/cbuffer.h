#pragma once

#include "intf/io.h"
#include "intf/types.h"

typedef struct
{
  u8 *data;
  u32 cap;
  u32 head;
  u32 tail;
  bool isfull;
} cbuffer;

#define cbuffer_create_from(data) cbuffer_create ((u8 *)data, sizeof data)
cbuffer cbuffer_create (u8 *data, u32 cap);

///////////////////////// Utils
bool cbuffer_isempty (const cbuffer *b);
u32 cbuffer_len (const cbuffer *b);
u32 cbuffer_avail (const cbuffer *b);
void cbuffer_discard_all (cbuffer *b);

///////////////////////// Raw Read / Write
u32 cbuffer_read (void *dest, u32 size, u32 n, cbuffer *b);
u32 cbuffer_copy (void *dest, u32 size, u32 n, const cbuffer *b);
u32 cbuffer_write (const void *src, u32 size, u32 n, cbuffer *b);

///////////////////////// CBuffer Read / Write
u32 cbuffer_cbuffer_move (cbuffer *dest, u32 size, u32 n, cbuffer *src);
u32 cbuffer_cbuffer_copy (cbuffer *dest, u32 size, u32 n, const cbuffer *src);

///////////////////////// IO Read / Write

/**
 * Reads in from [src] and writes to [b]
 * Errors:
 *   - ERR_IO - read error
 */
i32 cbuffer_write_some_from_file (i_file *src, cbuffer *b, error *e);

/**
 * Reads in from [b] and writes out to [dest]
 * Errors:
 *   - ERR_IO - write error
 */
i32 cbuffer_read_some_to_file (i_file *dest, cbuffer *b, error *e);

///////////////////////// Working with Single Elements
bool cbuffer_get (u8 *dest, const cbuffer *b, int idx);
bool cbuffer_peek_dequeue (u8 *dest, const cbuffer *b);
bool cbuffer_enqueue (cbuffer *b, u8 val);
bool cbuffer_dequeue (u8 *dest, cbuffer *b);
