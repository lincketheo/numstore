#pragma once

#include "dev/assert.h"
#include "intf/mm.h"
#include "types.h"

/**
 * SDS - Simple Data Structures
 * Minimal dependency data structures
 * that are useful abstractions
 */

//////////////////////////////// CBUFFER
/**
 * A cbuffer is a circular buffer.
 */
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

//////////////////////////////// BYTES

typedef struct
{
  u8 *data;
  u32 cap;
} bytes;

bytes bytes_create (u8 *data, u32 cap);
u32 bytes_write (const void *dest, u32 size, u32 n, bytes *b);

//////////////////////////////// STRINGS

typedef struct
{
  char *data;
  u32 len;
} string;

string unsafe_cstrfrom (char *cstr);
int strings_all_unique (const string *strs, u32 count);
bool string_equal (const string s1, const string s2);

//////////////////////////////// Array Indexing

typedef struct
{
  u32 start;
  u32 stop;
  int step;
} array_range;
