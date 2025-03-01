#include "compiler.h"
#include "dev/assert.h"
#include "intf/logging.h"
#include "intf/mm.h"
#include "intf/stdlib.h"
#include "sds.h"
#include "utils.h"

////////////////////// SCANNER (chars -> tokens)
DEFINE_DBG_ASSERT_I (scanner, scanner, s)
{
  ASSERT (s);
  cbuffer_assert (s->chars_input);
  cbuffer_assert (s->tokens_output);
}

void
scanner_create (scanner *dest, cbuffer *input, cbuffer *output)
{
  ASSERT (dest);
  i_memset (dest, 0, sizeof (scanner));

  ASSERT (output->cap % sizeof (token) == 0);
  dest->chars_input = input;
  dest->tokens_output = output;
  dest->current = 0;
  dest->is_error = 0;
}

static inline void
compile_error (scanner *s)
{
  scanner_assert (s);
  s->is_error = 1;
}

static inline void
consume_head_whitespace (scanner *s)
{
  scanner_assert (s);
  u8 c;

  while (cbuffer_peek_dequeue (&c, s->chars_input))
    {
      switch (c)
        {
        case ' ':
        case '\t':
        case '\r':
        case '\n':
          cbuffer_dequeue (&c, s->chars_input);
          break;
        default:
          return;
        }
    }
}

static inline int
advance_head (u8 *dest, scanner *s)
{
  scanner_assert (s);
  if (cbuffer_get (dest, s->chars_input, s->current))
    {
      s->current++;
      return 1;
    }
  return 0;
}

static inline void
consume_head (scanner *s)
{
  u8 ret;
  ASCOPE (int more =)
  advance_head (&ret, s);
  ASSERT (more);
}

static inline int
peek (u8 *dest, scanner *s)
{
  scanner_assert (s);
  return cbuffer_get (dest, s->chars_input, s->current);
}

static inline void
scanner_write_token (string *dest, scanner *s, token t)
{
  scanner_assert (s);
  ASSERT (cbuffer_avail (s->tokens_output) >= sizeof (token));

  ASCOPE (u32 ret =)
  cbuffer_write (&t, sizeof t, 1, s->tokens_output);
  ASSERT (ret == 1);

  ASCOPE (u32 read);
  if (dest)
    {
      ASSERT (dest->len == s->current);
      ASCOPE (read =)
      cbuffer_read (dest->data, 1, s->current, s->chars_input);
    }
  else
    {
      ASCOPE (read =)
      cbuffer_read (NULL, 1, s->current, s->chars_input);
    }
  ASSERT (read == (u32)s->current);
  s->current = 0;
}

static inline void
scanner_write_token_t (scanner *s, token_t t)
{
  scanner_assert (s);
  ASSERT (cbuffer_avail (s->tokens_output) >= sizeof (token));
  ASSERT (TT_IS_SINGLE (t));
  token _t;
  _t.type = t;
  scanner_write_token (NULL, s, _t);
}

static inline int
scan_next_ident_token (scanner *s)
{
  scanner_assert (s);
  u8 c;
  token dest;

  while (peek (&c, s))
    {
      if (!is_alpha_num (c))
        {
          goto finish;
        }
      consume_head (s);
    }

  return 0;

finish:
  scanner_assert (s);

  // Check for special tokens
  if (cbuffer_strequal (s->chars_input, "write", 5))
    {
      dest.type = TT_WRITE;
      scanner_write_token (NULL, s, dest);
    }
  else if (cbuffer_strequal (s->chars_input, "u32", 3))
    {
      dest.type = TT_U32;
      scanner_write_token (NULL, s, dest);
    }
  else
    {
      dest.type = TT_IDENTIFIER;

      // TODO:OPTIMIZATION reduce mallocs
      char *str = i_malloc (s->current);
      dest.str = (string){
        .data = str,
        .len = s->current,
      };
      scanner_write_token (&dest.str, s, dest);
    }

  return 1;
}

static inline int
scan_next_number_token (scanner *s)
{
  scanner_assert (s);

  u8 c;
  token dest;

  while (peek (&c, s))
    {
      if (!is_num (c))
        {

          if (c == '.')
            {
              consume_head (s);
              goto scan_float;
            }
          else
            {
              int ret = cbuffer_parse_front_i32 (
                  &dest.integer,
                  s->chars_input,
                  s->current);
              if (ret)
                {
                  compile_error (s);
                  return 1;
                }
              dest.type = TT_INTEGER;
              scanner_write_token (NULL, s, dest);
              return 1;
            }
        }
      consume_head (s);
    }

  return 0;

scan_float:

  while (peek (&c, s))
    {
      if (!is_num (c))
        {
          int ret = cbuffer_parse_front_f32 (
              &dest.floating,
              s->chars_input,
              s->current);
          if (ret)
            {
              compile_error (s);
              return 1;
            }
          dest.type = TT_FLOAT;
          scanner_write_token (NULL, s, dest);
          return 1;
        }
      consume_head (s);
    }

  return 0;
}

static inline int
scan_next_token (scanner *s)
{
  scanner_assert (s);

  // Needs enough room for 1 token
  if (cbuffer_avail (s->tokens_output) < sizeof (token))
    {
      return 0;
    }

  // Get next char (start of this token)
  u8 next;
  if (!advance_head (&next, s))
    {
      return 0; // Nothing to scan, no chars available
    }

  switch (next)
    {
    case ';':
      {
        scanner_write_token_t (s, TT_SEMICOLON);
        return 1;
      }
    default:
      {
        if (is_alpha (next))
          {
            return scan_next_ident_token (s);
          }
        else if (is_num (next) || next == '+' || next == '-')
          {
            return scan_next_number_token (s);
          }
        compile_error (s);
        return 0;
      }
    }
}

void
scanner_execute (scanner *s)
{
  scanner_assert (s);
  consume_head_whitespace (s);
  scan_next_token (s);
  consume_head_whitespace (s);
}

////////////////////// TOKEN PRINTER
void
token_log_info (token t)
{
  switch (t.type)
    {
    case TT_WRITE:
      i_log_info ("WRITE\n");
      return;
    case TT_IDENTIFIER:
      i_log_info ("IDENTIFIER %.*s\n", t.str.len, t.str.data);
      return;
    case TT_INTEGER:
      i_log_info ("INTEGER %d\n", t.integer);
      return;
    case TT_FLOAT:
      i_log_info ("FLOAT %f\n", t.floating);
      return;
    case TT_U32:
      i_log_info ("U32\n");
      return;
    case TT_SEMICOLON:
      i_log_info ("SEMICOLON\n");
      return;
    }
}

void
token_printer_create (token_printer *dest, cbuffer *input)
{
  ASSERT (dest);
  i_memset (dest, 0, sizeof (token_printer));

  // TODO:OPTIMIZATION - reduce malloc
  ASSERT (input->cap % sizeof (token) == 0);
  dest->tokens_input = input;
}

void
token_printer_execute (token_printer *t)
{
  token next;
  while (cbuffer_read (&next, sizeof (token), 1, t->tokens_input))
    {
      token_log_info (next);
    }
}

////////////////////// PARSER
DEFINE_DBG_ASSERT_H (parser, parser, p)
{
  ASSERT (p);
  ASSERT (p->tokens_input);
  ASSERT (p->bytecode_output);
  ASSERT (p->is_error == 1 || p->is_error == 0);
}

void
parser_create (parser *dest, cbuffer *input, cbuffer *output)
{
  ASSERT (dest);
  i_memset (dest, 0, sizeof (scanner));

  dest->tokens_input = input;
  dest->bytecode_output = output;
  dest->is_error = 0;
}

void
parser_execute (parser *s)
{
  parser_assert (s);

  // TODO
  token t;
  ASCOPE (int read =)
  cbuffer_read (&t, sizeof (t), 1, s->tokens_input);
  ASSERT (read == 1);
}
