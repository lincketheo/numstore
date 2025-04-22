#include "compiler.h"
#include "dev/assert.h"
#include "dev/errors.h"
#include "dev/testing.h"
#include "intf/logging.h"
#include "intf/mm.h"
#include "intf/stdlib.h"
#include "sds.h"
#include "utils/macros.h"
#include "utils/numbers.h"

////////////////////// SCANNER (chars -> tokens)
DEFINE_DBG_ASSERT_I (scanner, scanner, s)
{
  ASSERT (s);
  cbuffer_assert (s->chars_input);
  cbuffer_assert (s->tokens_output);
}

static inline bool
scanner_alloc_init (scanner *s)
{
  scanner_assert (s);
  ASSERT (s->dcur == NULL);
  ASSERT (s->dcurlen == 0);
  ASSERT (s->dcurcap == 0);

#define STARTING_SIZE 10 // TODO - analyze common string lengths?

  char *data = lmalloc (s->alloc, STARTING_SIZE);
  if (!data)
    {
      return false; // no memory
    }

  s->dcur = data;
  s->dcurcap = STARTING_SIZE;
  s->dcurlen = 0;

  return true;
}

// Doesn't call free
static inline void
scanner_buffer_reset (scanner *s)
{
  scanner_assert (s);
  ASSERT (s->dcur != NULL);
  ASSERT (s->dcurlen > 0);
  ASSERT (s->dcurcap > 0);

  s->dcur = NULL;
  s->dcurlen = 0;
  s->dcurcap = 0;
}

void
scanner_create (
    scanner *dest,
    cbuffer *input,
    cbuffer *output,
    lalloc *alloc)
{
  ASSERT (dest);
  ASSERT (output->cap % sizeof (token) == 0);
  lalloc_assert (alloc);

  dest->chars_input = input;
  dest->tokens_output = output;
  dest->state = SS_START;
  dest->current = 0;

  dest->dcur = NULL;
  dest->dcurlen = 0;
  dest->dcurcap = 0;
  dest->alloc = alloc;
}

static inline void
scanner_advance_expect (scanner *s)
{
  scanner_assert (s);

  u8 next;
  ASCOPE (int more =)
  cbuffer_dequeue (&next, s->chars_input);
  ASSERT (more);
}

static inline bool
scanner_cpy_advance_expect (scanner *s)
{
  scanner_assert (s);
  ASSERT (s->dcur != NULL);
  ASSERT (s->dcurcap > 0);

  // Dequeue and copy
  u8 next;
  ASCOPE (int more =)
  cbuffer_dequeue (&next, s->chars_input);
  ASSERT (more);

  // Check room
  if (s->dcurlen == s->dcurcap)
    {
      char *next = lrealloc (s->alloc, s->dcur, 2 * s->dcurcap);
      if (!next)
        {
          return false; // No memory - block
        }
      s->dcur = next;
    }

  s->dcur[s->dcurlen++] = next;

  return true;
}

static inline bool
scanner_cpy_advance (scanner *s)
{
  scanner_assert (s);
  ASSERT (s->dcur != NULL);
  ASSERT (s->dcurcap > 0);

  // Dequeue and copy
  u8 next;
  if (!cbuffer_dequeue (&next, s->chars_input))
    {
      return false;
    }

  // Check room
  if (s->dcurlen == s->dcurcap)
    {
      char *dcur = lrealloc (s->alloc, s->dcur, 2 * s->dcurcap);
      if (!dcur)
        {
          return false; // No memory - block
        }
      s->dcur = dcur;
    }

  s->dcur[s->dcurlen++] = next;

  return true;
}

static inline int
scanner_peek (u8 *dest, scanner *s)
{
  scanner_assert (s);
  return cbuffer_peek_dequeue (dest, s->chars_input);
}

static inline void
consume_head_whitespace (scanner *s)
{
  scanner_assert (s);
  u8 c;

  while (scanner_peek (&c, s))
    {
      switch (c)
        {
        case ' ':
        case '\t':
        case '\r':
        case '\n':
          scanner_advance_expect (s);
          break;
        default:
          return;
        }
    }
}

static inline void
scanner_write_token (scanner *s, token t)
{
  scanner_assert (s);
  ASSERT (cbuffer_avail (s->tokens_output) >= sizeof (token));

  ASCOPE (u32 ret =)
  cbuffer_write (&t, sizeof t, 1, s->tokens_output);
  ASSERT (ret == 1);
}

static inline void
scanner_write_token_t (scanner *s, token_t t)
{
  scanner_assert (s);
  ASSERT (cbuffer_avail (s->tokens_output) >= sizeof (token));
  scanner_write_token (s, quick_tok (t));
}

typedef struct
{
  char *data;
  u32 len;
  token_t type;
} magic_token;

static const magic_token magic_tokens[] = {
  {
      .data = "write",
      .len = 5,
      .type = TT_WRITE,
  },
  {
      .data = "create",
      .len = 6,
      .type = TT_CREATE,
  },
  {
      .data = "u32",
      .len = 3,
      .type = TT_U32,
  },
};

static void ss_transition (scanner *s, scanner_state state);
static void ss_start (scanner *s);
static void ss_char_collect (scanner *s);
static void ss_number_collect (scanner *s);
static void ss_decimal_collect (scanner *s);
static void ss_error_rewind (scanner *s);

static void
ss_transition (scanner *s, scanner_state state)
{
  scanner_assert (s);
  s->state = state;
  if (cbuffer_avail (s->tokens_output) < sizeof (token))
    {
      return;
    }
  switch (state)
    {
    case SS_START:
      ss_start (s);
      break;
    case SS_CHAR_COLLECT:
      ss_char_collect (s);
      break;
    case SS_NUMBER_COLLECT:
      ss_number_collect (s);
      break;
    case SS_DECIMAL_COLLECT:
      ss_decimal_collect (s);
      break;
    case SS_ERROR_REWIND:
      ss_error_rewind (s);
      break;
    }
}

// starts on the SECOND char of the sequence
static void
ss_char_collect (scanner *s)
{
  scanner_assert (s);
  ASSERT (s->state == SS_CHAR_COLLECT);

  u8 c;

  while (scanner_peek (&c, s))
    {
      if (!is_alpha_num (c))
        {
          goto finish;
        }

      if (!scanner_cpy_advance_expect (s))
        {
          return;
        }
    }

  return;

finish:
  scanner_assert (s);

  ASSERT (s->dcurlen > 0);
  ASSERT (s->dcur);

  string literal = (string){
    .data = s->dcur,
    .len = s->dcurlen,
  };

  /**
   * WARNING - This looks like it's causing
   * a dangling pointer but really I'm transferring
   * responsibility to the next block
   */
  scanner_buffer_reset (s);
  s->state = SS_START;

  // Check for special tokens
  for (u32 i = 0; i < arrlen (magic_tokens); ++i)
    {
      if (string_equal (
              literal, (string){
                           .data = magic_tokens[i].data,
                           .len = magic_tokens[i].len,
                       }))
        {
          scanner_write_token_t (s, magic_tokens[i].type);
          lfree (s->alloc, literal.data);
          goto theend;
        }
    }

  scanner_write_token (s, tt_ident (literal));

theend:
  ss_transition (s, SS_START);
  return;
}

static void
ss_decimal_collect (scanner *s)
{
  scanner_assert (s);
  ASSERT (s->state == SS_DECIMAL_COLLECT);

  u8 c;
  while (scanner_peek (&c, s))
    {
      if (!is_num (c))
        {
          const string literal = (string){
            .data = s->dcur,
            .len = s->dcurlen,
          };

          // Parse the int
          f32 dest;
          err_t ret = parse_f32_expect (&dest, literal);
          if (ret)
            {
              ss_transition (s, SS_ERROR_REWIND);
              return;
            }

          // Reset and write
          lfree (s->alloc, s->dcur);
          scanner_buffer_reset (s);
          scanner_write_token (s, tt_float (dest));

          // TODO - should I require whitespace after numbers?
          ss_transition (s, SS_START);
          return;
        }
      if (!scanner_cpy_advance_expect (s))
        {
          return;
        }
    }
}

static void
ss_error_rewind (scanner *s)
{
  scanner_assert (s);
  ASSERT (s->state == SS_ERROR_REWIND);

  // For now, just blindly consume all
  s->chars_input->head = 0;
  s->chars_input->tail = 0;
  s->chars_input->isfull = false;
}

// Starts on the SECOND char of the sequence
static void
ss_number_collect (scanner *s)
{
  scanner_assert (s);
  ASSERT (s->state == SS_NUMBER_COLLECT);

  u8 c;

  while (scanner_peek (&c, s))
    {
      if (!is_num (c))
        {

          if (c == '.')
            {
              if (!scanner_cpy_advance_expect (s))
                {
                  return;
                }

              ss_transition (s, SS_DECIMAL_COLLECT);
              return;
            }
          else
            {
              const string literal = (string){
                .data = s->dcur,
                .len = s->dcurlen,
              };

              // Parse the int
              i32 dest;
              err_t ret = parse_i32_expect (&dest, literal);
              if (ret)
                {
                  ss_transition (s, SS_ERROR_REWIND);
                  return;
                }

              // Reset and write
              lfree (s->alloc, s->dcur);
              scanner_buffer_reset (s);
              scanner_write_token (s, tt_integer (dest));

              ss_transition (s, SS_START);
              return;
            }
        }
      if (!scanner_cpy_advance_expect (s))
        {
          return;
        }
    }

  return;
}

static void
ss_start (scanner *s)
{
  scanner_assert (s);
  ASSERT (s->state == SS_START);

  // Consume all whitespace
  consume_head_whitespace (s);

  // First check if there's data available
  u8 next;
  if (!cbuffer_peek_dequeue (&next, s->chars_input))
    {
      return;
    }

  switch (next)
    {
    case ';':
      {
        scanner_write_token_t (s, TT_SEMICOLON);
        scanner_advance_expect (s);
        ss_transition (s, SS_START);
        return;
      }
    default:
      {

        if (is_alpha (next))
          {
            // No room in allocator
            if (!scanner_alloc_init (s))
              {
                return;
              }
            if (!scanner_cpy_advance (s))
              {
                return;
              }

            // Move onto char collect
            ss_transition (s, SS_CHAR_COLLECT);
            return;
          }
        else if (is_num (next) || next == '+' || next == '-')
          {
            // No room in allocator
            if (!scanner_alloc_init (s))
              {
                return;
              }
            if (!scanner_cpy_advance (s))
              {
                return;
              }

            ss_transition (s, SS_NUMBER_COLLECT);
            return;
          }
        else
          {
            ss_transition (s, SS_ERROR_REWIND);
            return;
          }
      }
    }
}

void
scanner_execute (scanner *s)
{
  scanner_assert (s);

  // Pick up where you left off
  switch (s->state)
    {
    case SS_START:
      {
        ss_transition (s, SS_START);
        return;
      }
    case SS_CHAR_COLLECT:
      {
        ss_transition (s, SS_CHAR_COLLECT);
        return;
      }
    case SS_NUMBER_COLLECT:
      {
        ss_transition (s, SS_NUMBER_COLLECT);
        return;
      }
    case SS_DECIMAL_COLLECT:
      {
        ss_transition (s, SS_DECIMAL_COLLECT);
        return;
      }
    case SS_ERROR_REWIND:
      {
        ss_transition (s, SS_ERROR_REWIND);
        return;
      }
    }
}

#ifndef NTESTS
void
test_token_compare (const token t1, const token t2)
{
  test_assert_int_equal (t1.type, t2.type);
  switch (t1.type)
    {
    case TT_FLOAT:
      test_assert_equal (t1.floating, t2.floating, "%f");
      break;
    case TT_INTEGER:
      test_assert_int_equal (t1.integer, t2.integer);
      break;
    case TT_IDENTIFIER:
      test_fail_if (!string_equal (t1.str, t2.str));
      break;
    default:
      break;
    }
}

void
scanner_test_helper (
    const char *str,
    const token *tokens,
    u32 len,
    u32 mlimit,
    u32 inlen,
    u32 outlen)
{
  lalloc alloc = lalloc_create (mlimit + inlen + outlen);

  u8 *_input = lmalloc (&alloc, inlen);
  test_fail_if_null (_input);
  u8 *_output = lmalloc (&alloc, outlen);
  test_fail_if_null (_output);

  cbuffer input = cbuffer_create (_input, inlen);
  cbuffer output = cbuffer_create (_output, outlen);

  scanner s;
  scanner_create (&s, &input, &output, &alloc);

  u32 i = 0;

  while (i_unsafe_strlen (str) > 0)
    {
      str += cbuffer_write (str, 1, i_unsafe_strlen (str), &input);
      scanner_execute (&s);
      while (cbuffer_len (&output) > 0)
        {
          token next;
          u32 read = cbuffer_read (&next, sizeof (token), 1, &output);
          test_assert_int_equal (read, 1);
          test_fail_if (i >= len);
          test_token_compare (next, tokens[i]);
          i += 1;
          if (next.type == TT_IDENTIFIER)
            {
              lfree (&alloc, next.str.data);
            }
        }
    }
  test_assert_int_equal (i, len);

  // No memory leaks
  test_assert_int_equal (alloc.total, 2 * 8 + inlen + outlen);
}
#endif
TEST (scanner_execute)
{
  const char *str = "write 5 ; ; ;;create123create createc "
                    "create 123createc 123create 12345.1create "
                    "u32 write foo bar biz buz "; // space to terminate
  const token tokens[] = {
    quick_tok (TT_WRITE),
    tt_integer (5),
    quick_tok (TT_SEMICOLON),
    quick_tok (TT_SEMICOLON),
    quick_tok (TT_SEMICOLON),
    quick_tok (TT_SEMICOLON),
    tt_ident (unsafe_cstrfrom ("create123create")),
    tt_ident (unsafe_cstrfrom ("createc")),
    quick_tok (TT_CREATE),
    tt_integer (123),
    tt_ident (unsafe_cstrfrom ("createc")),
    tt_integer (123),
    quick_tok (TT_CREATE),
    tt_float (12345.1),
    quick_tok (TT_CREATE),
    quick_tok (TT_U32),
    quick_tok (TT_WRITE),
    tt_ident (unsafe_cstrfrom ("foo")),
    tt_ident (unsafe_cstrfrom ("bar")),
    tt_ident (unsafe_cstrfrom ("biz")),
    tt_ident (unsafe_cstrfrom ("buz")),
  };

  scanner_test_helper (str, tokens, arrlen (tokens), 100, 10, 2 * sizeof (token));
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
