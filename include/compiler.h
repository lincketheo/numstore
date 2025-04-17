#pragma once

// write a U32;...
#include "buffer.h"
#include "core/string.h"
#include "dev/assert.h"

#include <ctype.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef enum
{
  TT_WRITE,
  TT_IDENTIFIER,
  TT_U32,
  TT_SEMICOLON,
  TT_EOF
} token_t;

typedef struct
{
  union
  {
    string str;
  };
  token_t type;
} token;

typedef struct
{
  buffer *input;  // Char input
  buffer *output; // Byte Code output
  int start;
  int cur;
  int col;
  int is_error;
} scanner;

static inline DEFINE_DBG_ASSERT_I (scanner, scanner, s)
{
  ASSERT (s);
  buffer_assert (s->input);
  buffer_assert (s->output);
}

void
scanner_create (scanner *dest, buffer *input, buffer *output)
{
  ASSERT (dest);
  buffer_assert (input);
  dest->input = input;
  dest->output = output;
  dest->start = 0;
  dest->cur = 0;
  dest->col = 0;
  dest->is_error = 0;
}

static inline char
scanner_peek (scanner *s)
{
  scanner_assert (s);
  return 'c';
}

static inline void
scanner_skip_whitespace (scanner *s)
{
  scanner_assert (s);
  while (1)
    {
    }
}

void
scan_all (scanner *s)
{
  scanner_assert (s);
  while (1)
    {
    }
}

inline void
scanner_advance ()
{
}

static void
advance (void)
{
}
