#include "compiler.h"
#include "dev/assert.h"
#include "dev/errors.h"
#include "dev/testing.h"
#include "intf/logging.h"
#include "intf/mm.h"
#include "intf/stdlib.h"
#include "sds.h"
#include "typing.h"
#include "utils/macros.h"
#include "utils/numbers.h"
#include <stdio.h>
#include <stdlib.h>

////////////////////// SCANNER (chars -> tokens)
/// TODO - This recursion is really easy to remove and
/// is unecessary - helped me think
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

  char *data = lmalloc (s->strings_alloc, STARTING_SIZE);
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
    lalloc *strings_alloc)
{
  ASSERT (dest);
  ASSERT (output->cap % sizeof (token) == 0);
  cbuffer_assert (input);
  cbuffer_assert (output);
  lalloc_assert (strings_alloc);

  dest->chars_input = input;
  dest->tokens_output = output;
  dest->state = SS_START;
  dest->current = 0;

  dest->dcur = NULL;
  dest->dcurlen = 0;
  dest->dcurcap = 0;
  dest->strings_alloc = strings_alloc;
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
      char *next = lrealloc (s->strings_alloc, s->dcur, 2 * s->dcurcap);
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
      char *dcur = lrealloc (s->strings_alloc, s->dcur, 2 * s->dcurcap);
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
  prim_t type;
} prim_token;

static const prim_token prim_tokens[] = {
  { .data = "u8", .len = 2, .type = U8 },
  { .data = "u16", .len = 3, .type = U16 },
  { .data = "u32", .len = 3, .type = U32 },
  { .data = "u64", .len = 3, .type = U64 },

  { .data = "i8", .len = 2, .type = I8 },
  { .data = "i16", .len = 3, .type = I16 },
  { .data = "i32", .len = 3, .type = I32 },
  { .data = "i64", .len = 3, .type = I64 },

  { .data = "f16", .len = 3, .type = F16 },
  { .data = "f32", .len = 3, .type = F32 },
  { .data = "f64", .len = 3, .type = F64 },
  { .data = "f128", .len = 4, .type = F128 },

  { .data = "cf64", .len = 4, .type = CF64 },
  { .data = "cf128", .len = 5, .type = CF128 },
  { .data = "cf256", .len = 5, .type = CF256 },

  { .data = "ci16", .len = 4, .type = CI16 },
  { .data = "ci32", .len = 4, .type = CI32 },
  { .data = "ci64", .len = 4, .type = CI64 },
  { .data = "ci128", .len = 5, .type = CI128 },

  { .data = "cu16", .len = 4, .type = CU16 },
  { .data = "cu32", .len = 4, .type = CU32 },
  { .data = "cu64", .len = 4, .type = CU64 },
  { .data = "cu128", .len = 5, .type = CU128 },

  { .data = "bool", .len = 4, .type = BOOL },
  { .data = "bit", .len = 3, .type = BIT },
};

typedef struct
{
  char *data;
  u32 len;
  token_t type;
} magic_token;

/**
 * TODO - this search can
 * be simplified by checking the first letter
 * Same logically, but less function calls to string_equal
 */
static const magic_token magic_tokens[] = {
  { .data = "write", .len = 5, .type = TT_WRITE },
  { .data = "read", .len = 4, .type = TT_READ },
  { .data = "take", .len = 4, .type = TT_TAKE },
  { .data = "create", .len = 6, .type = TT_CREATE },
  { .data = "delete", .len = 6, .type = TT_DELETE },

  // Complex types
  { .data = "struct", .len = 6, .type = TT_STRUCT },
  { .data = "union", .len = 5, .type = TT_UNION },
  { .data = "enum", .len = 4, .type = TT_ENUM },
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
      {
        ss_start (s);
        break;
      }
    case SS_CHAR_COLLECT:
      {
        ss_char_collect (s);
        break;
      }
    case SS_NUMBER_COLLECT:
      {
        ss_number_collect (s);
        break;
      }
    case SS_DECIMAL_COLLECT:
      {
        ss_decimal_collect (s);
        break;
      }
    case SS_ERROR_REWIND:
      {
        ss_error_rewind (s);
        break;
      }
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

  // Check for magic tokens
  for (u32 i = 0; i < arrlen (magic_tokens); ++i)
    {
      if (string_equal (
              literal, (string){
                           .data = magic_tokens[i].data,
                           .len = magic_tokens[i].len,
                       }))
        {
          scanner_write_token_t (s, magic_tokens[i].type);
          lfree (s->strings_alloc, literal.data);
          goto theend;
        }
    }

  // Check for primitives
  for (u32 i = 0; i < arrlen (prim_tokens); ++i)
    {
      if (string_equal (
              literal, (string){
                           .data = prim_tokens[i].data,
                           .len = prim_tokens[i].len,
                       }))
        {
          scanner_write_token (s, tt_prim (prim_tokens[i].type));
          lfree (s->strings_alloc, literal.data);
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
          lfree (s->strings_alloc, s->dcur);
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
              lfree (s->strings_alloc, s->dcur);
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
  if (!scanner_peek (&next, s))
    {
      return;
    }

#define single_tok_continue(ttype)  \
  scanner_write_token_t (s, ttype); \
  scanner_advance_expect (s);       \
  ss_transition (s, SS_START)

  switch (next)
    {
    case ';':
      {
        single_tok_continue (TT_SEMICOLON);
        return;
      }
    case '[':
      {
        single_tok_continue (TT_LEFT_BRACKET);
        return;
      }
    case ']':
      {
        single_tok_continue (TT_RIGHT_BRACKET);
        return;
      }
    case '{':
      {
        single_tok_continue (TT_LEFT_BRACE);
        return;
      }
    case '}':
      {
        single_tok_continue (TT_RIGHT_BRACE);
        return;
      }
    case '(':
      {
        single_tok_continue (TT_LEFT_PAREN);
        return;
      }
    case ')':
      {
        single_tok_continue (TT_RIGHT_PAREN);
        return;
      }
    case ',':
      {
        single_tok_continue (TT_COMMA);
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
#undef single_tok_continue
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
/**
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
    tt_prim (U32),
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
  cbuffer_assert (p->tokens_input);
  cbuffer_assert (p->opcode_output);
  lalloc_assert (p->input_strings);
  lalloc_assert (p->tokens);
}

void
parser_create (
    parser *dest,
    cbuffer *input,
    cbuffer *output,
    lalloc *input_strings,
    lalloc *output_types)
{
  ASSERT (dest);
  lalloc_assert (input_strings);
  lalloc_assert (output_types);
  cbuffer_assert (input);
  cbuffer_assert (output);

  i_memset (dest, 0, sizeof (scanner));

  dest->tokens_input = input;
  dest->opcode_output = output;
  dest->current_type = NULL;
  dest->state = PS_START;

  parser_assert (dest);
}

static inline int
parser_peek (token *dest, parser *p)
{
  parser_assert (p);
  return cbuffer_copy (dest, sizeof *dest, 1, p->tokens_input);
}

static void ps_transition (parser *p, parser_state state);

static void
ps_start (parser *p)
{
  parser_assert (p);
  ASSERT (p->state == PS_START);

  token next;
  if (!parser_peek (&next, p))
    {
      return;
    }

  switch (next.type)
    {
    case TT_CREATE:
      {
        return;
      }
    default:
      {
        ps_transition (p, PS_ERROR_REWIND);
        return;
      }
    }
}

static void
ps_transition (parser *p, parser_state state)
{
  parser_assert (p);
  p->state = state;
  if (cbuffer_avail (p->tokens_output) < sizeof (token))
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

static void
ps_start (parser *p)
{
  parser_assert (p);
}

void
parser_execute (parser *p)
{
  parser_assert (p);

  switch (p->state)
    {
    case PS_START:
      return;
    }
}
*/

////////////////////// TYPE PARSER

tp_result
sb_accept_token (type_builder *sb, token t)
{
  switch (sb->sb.state)
    {
    // { -> IDENT
    case SB_WAITING_FOR_LB:
      {
        if (t.type != TT_LEFT_BRACE)
          {
            return TPR_SYNTAX_ERROR;
          }
        sb->sb.state = SB_WAITING_FOR_IDENT;
        return TPR_EXPECT_NEXT_TOKEN;
      }

    case SB_WAITING_FOR_IDENT:
      {
        if (t.type != TT_IDENTIFIER)
          {
            return TPR_SYNTAX_ERROR;
          }
        sb->ret->st->keys[sb->ret->st->len] = t.str;
        sb->sb.state = SB_WAITING_FOR_TYPE;
        return TPR_EXPECT_NEXT_TYPE;
      }

    case SB_WAITING_FOR_COMMA_OR_RIGHT:
      {
        if (t.type == TT_COMMA)
          {
            sb->sb.state = SB_WAITING_FOR_IDENT;
            return TPR_EXPECT_NEXT_TOKEN;
          }
        else if (t.type == TT_RIGHT_BRACE)
          {
            return TPR_DONE;
          }
        else
          {
            return TPR_SYNTAX_ERROR;
          }
      }

    default:
      {
        return TPR_SYNTAX_ERROR;
      }
    }
}

tp_result
sb_accept_type (type_builder *sb, type *type)
{
  switch (sb->sb.state)
    {
    case SB_WAITING_FOR_TYPE:
      {
        sb->ret->st->types[sb->ret->st->len++] = *type;
        free (type);
        sb->sb.state = SB_WAITING_FOR_COMMA_OR_RIGHT;
        return TPR_EXPECT_NEXT_TOKEN;
      }
    default:
      {
        return TPR_SYNTAX_ERROR;
      }
    }
}

tp_result
ub_accept_token (type_builder *ub, token t)
{
  switch (ub->ub.state)
    {
    // { -> IDENT
    case UB_WAITING_FOR_LB:
      {
        if (t.type != TT_LEFT_BRACE)
          {
            return TPR_SYNTAX_ERROR;
          }
        ub->ub.state = UB_WAITING_FOR_IDENT;
        return TPR_EXPECT_NEXT_TOKEN;
      }

    // IDENT -> TYPE
    case UB_WAITING_FOR_IDENT:
      {
        if (t.type != TT_IDENTIFIER)
          {
            return TPR_SYNTAX_ERROR;
          }

        // Ident is first, type increments len
        ub->ret->un->keys[ub->ret->un->len] = t.str;
        ub->ub.state = UB_WAITING_FOR_TYPE;
        return TPR_EXPECT_NEXT_TYPE;
      }

    // , -> IDENT
    // } -> DONE
    case UB_WAITING_FOR_COMMA_OR_RIGHT:
      {
        if (t.type == TT_COMMA)
          {
            ub->ub.state = UB_WAITING_FOR_IDENT;
            return TPR_EXPECT_NEXT_TOKEN;
          }
        else if (t.type == TT_RIGHT_BRACE)
          {
            return TPR_DONE;
          }
        else
          {
            return TPR_SYNTAX_ERROR;
          }
      }

    default:
      {
        return TPR_SYNTAX_ERROR;
      }
    }
}

tp_result
ub_accept_type (type_builder *ub, type *type)
{
  switch (ub->ub.state)
    {
    case UB_WAITING_FOR_TYPE:
      {
        ub->ret->un->types[ub->ret->un->len++] = *type;
        free (type);
        ub->ub.state = UB_WAITING_FOR_COMMA_OR_RIGHT;
        return TPR_EXPECT_NEXT_TOKEN;
      }
    default:
      {
        return TPR_SYNTAX_ERROR;
      }
    }
}

tp_result
tb_accept_token (type_builder *tb, token t)
{
  switch (tb->type)
    {
    case TB_UNSURE:
      {
        switch (t.type)
          {
          case TT_STRUCT:
            {
              tb->ret->st = malloc (sizeof *tb->ret->st);
              tb->ret->st->keys = malloc (1000);
              tb->ret->st->types = malloc (1000);
              tb->ret->st->len = 0;
              tb->ret->type = T_STRUCT;

              tb->type = TB_STRUCT;
              tb->sb.state = SB_WAITING_FOR_LB;
              return TPR_EXPECT_NEXT_TOKEN;
            }
          case TT_UNION:
            {
              tb->ret->un = malloc (sizeof *tb->ret->st);
              tb->ret->un->keys = malloc (1000);
              tb->ret->un->types = malloc (1000);
              tb->ret->un->len = 0;
              tb->ret->type = T_UNION;

              tb->type = TB_UNION;
              tb->ub.state = UB_WAITING_FOR_LB;
              return TPR_EXPECT_NEXT_TOKEN;
            }
          case TT_PRIM:
            {
              tb->ret->p = t.prim;
              tb->ret->type = T_PRIM;
              return TPR_DONE;
            }
          default:
            {
              return TPR_SYNTAX_ERROR;
            }
          }
      }
    case TB_STRUCT:
      {
        return sb_accept_token (tb, t);
      }
    case TB_UNION:
      {
        return ub_accept_token (tb, t);
      }
    }
  ASSERT (0);
  return 0;
}

tp_result
tb_accept_type (type_builder *tb, type *t)
{
  switch (tb->type)
    {
    case TB_STRUCT:
      {
        return sb_accept_type (tb, t);
      }
    case TB_UNION:
      {
        return ub_accept_type (tb, t);
      }
    default:
      {
        return TPR_SYNTAX_ERROR;
      }
    }
}

tp_result
tp_feed_token (type_parser *tp, token t)
{
  // Pop top type builder
  type_builder *top = &tp->stack[tp->sp - 1];
  tp_result ret = tb_accept_token (top, t);

  // Top expects a new type, push a new type
  // builder to the stack
  if (ret == TPR_EXPECT_NEXT_TYPE)
    {
      type_builder next;
      next.ret = malloc (sizeof *next.ret);
      next.type = TB_UNSURE;
      tp->stack[tp->sp++] = next;
      return TPR_EXPECT_NEXT_TOKEN;
    }

  while (tp->sp > 1 && ret == TPR_DONE)
    {
      type_builder top = tp->stack[--tp->sp];
      type_builder *newtop = &tp->stack[tp->sp - 1];
      ret = tb_accept_type (newtop, top.ret);
    }

  return ret;
}

TEST (tp_feed_token)
{
  const char *str = "struct { a i32, b u32, c union { a bool, b bit, c cf128 } }";

  char chars_input[10];
  token tokens[10];
  cbuffer input = cbuffer_create ((u8 *)chars_input, 10);
  cbuffer output = cbuffer_create ((u8 *)tokens, 10 * sizeof *tokens);

  lalloc alloc = lalloc_create (0);

  scanner s;
  scanner_create (&s, &input, &output, &alloc);

  type_parser tp;
  tp.stack = malloc (1000);
  type_builder first;
  first.ret = malloc (sizeof *first.ret);
  first.type = TB_UNSURE;

  tp.sp = 0;
  tp.stack[tp.sp++] = first;

  while (i_unsafe_strlen (str) > 0)
    {
      str += cbuffer_write (str, 1, i_unsafe_strlen (str), &input);
      scanner_execute (&s);
      while (cbuffer_len (&output) > 0)
        {
          token next;
          cbuffer_read (&next, sizeof (token), 1, &output);
          tp_feed_token (&tp, next);
        }
    }
  i_log_type (tp.stack[0].ret);
}
