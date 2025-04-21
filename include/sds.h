#pragma once

#include "dev/assert.h"
#include "types.h"

/**
 * SDS - Simple Data Structures
 * Minimal dependency data structures
 * that are useful abstractions
 */

//////////////////////////////// BUFFER
/**
 * A buffer is an array of bytes with a capacity
 * and length
 */
typedef struct
{
  u8 *data;
  u32 cap;
  u32 len;
} buffer;

DEFINE_DBG_ASSERT_H (buffer, buffer, b);
buffer buffer_create (u8 *data, u32 cap);
void buffer_reset (buffer *b);
u32 buffer_write (const void *dest, u32 size, u32 n, buffer *b);
u32 buffer_phantom_write (u32 size, u32 n, buffer *b);

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
  int isfull;
} cbuffer;

DEFINE_DBG_ASSERT_H (cbuffer, cbuffer, b);
cbuffer cbuffer_create (u8 *data, u32 cap);
bool cbuffer_isempty (const cbuffer *b);
u32 cbuffer_len (const cbuffer *b);
u32 cbuffer_avail (const cbuffer *b);

// Consumes the tail
u32 cbuffer_read (void *dest, u32 size, u32 n, cbuffer *b);
u32 cbuffer_write (const void *src, u32 size, u32 n, cbuffer *b);
int cbuffer_get (u8 *dest, const cbuffer *b, int idx);
int cbuffer_peek_dequeue (u8 *dest, const cbuffer *b);
int cbuffer_enqueue (cbuffer *b, u8 val);
int cbuffer_dequeue (u8 *dest, cbuffer *b);

/**
 * Parsing functions
 * - Utilities for the compiler
 *   These three functions expect the front to be in a valid format
 *   E.g. they ASSERT on correct format.
 * - They do not consume the tail
 */
// Checks if the prefix is equal to [str]
// Does not consume the tail
int cbuffer_strequal (const cbuffer *c, const char *str, u32 strlen);

// Return ERR
// May still error on parsing (e.g. number overflow)
int cbuffer_parse_front_i32 (i32 *dest, const cbuffer *src, u32 until);
int cbuffer_parse_front_f32 (f32 *dest, const cbuffer *src, u32 until);

//////////////////////////////// BYTES
/**
 * bytes is just sized bytes
 */
typedef struct
{
  u8 *data;
  u32 cap;
} bytes;

DEFINE_DBG_ASSERT_H (bytes, bytes, b);
bytes bytes_create (u8 *data, u32 cap);
u32 bytes_write (const void *dest, u32 size, u32 n, bytes *b);

//////////////////////////////// STRINGS
typedef struct
{
  char *data;
  u32 len;
} string;

DEFINE_DBG_ASSERT_H (string, string, s);
DEFINE_DBG_ASSERT_H (string, cstring, s);
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

DEFINE_DBG_ASSERT_H (array_range, array_range, a);

//////////////////////////////// Linked List

typedef struct llist
{
  struct llist *next;
} llist;

DEFINE_DBG_ASSERT_H (llist, llist, l);
