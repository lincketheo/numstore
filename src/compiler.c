#include "compiler.h"
#include "dev/assert.h"
#include "dev/errors.h"
#include "dev/testing.h"
#include "intf/logging.h"
#include "intf/mm.h"
#include "intf/stdlib.h"
#include "sds.h"
#include "typing.h"
#include "utils/bounds.h"
#include "utils/macros.h"
#include "utils/numbers.h"

////////////////////// TOKENS

typedef enum
{
  // Tokens that start with a letter (alpha)
  TT_WRITE,
  TT_READ,
  TT_TAKE,
  TT_CREATE,
  TT_DELETE,
  TT_IDENTIFIER,

  // Tokens that start with a number or +/-
  TT_INTEGER,
  TT_FLOAT,

  // Types
  //      Complex
  TT_STRUCT,
  TT_UNION,
  TT_ENUM,
  TT_PRIM,

  // Tokens that are single characters
  TT_SEMICOLON,
  TT_LEFT_BRACKET,
  TT_RIGHT_BRACKET,
  TT_LEFT_BRACE,
  TT_RIGHT_BRACE,
  TT_LEFT_PAREN,
  TT_RIGHT_PAREN,
  TT_COMMA,

  // Special Tokens
  TT_ERROR, // An Error token, saying: next token start fresh
} token_t;

// Returns if this token is a single character
#define TT_IS_SINGLE(t) (t == TT_SEMICOLON)

/**
 * A token is a tagged union that wraps
 * the value and the type together
 */
typedef struct
{
  union
  {
    string str;
    i32 integer;
    f32 floating;
    prim_t prim;
  };
  token_t type;
} token;

#define quick_tok(_type) \
  (token) { .type = _type }
#define tt_integer(val) \
  (token) { .type = TT_INTEGER, .integer = val }
#define tt_float(val) \
  (token) { .type = TT_FLOAT, .floating = val }
#define tt_ident(val) \
  (token) { .type = TT_IDENTIFIER, .str = val }
#define tt_prim(val) \
  (token) { .type = TT_PRIM, .prim = val }

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
      char *next = lrealloc (s->strings_alloc, s->dcur, 2 * s->dcurcap * sizeof *next);
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
      char *dcur = lrealloc (s->strings_alloc, s->dcur, 2 * s->dcurcap * sizeof *dcur);
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

typedef struct type_bldr_s type_bldr;

typedef enum
{

  TPR_EXPECT_NEXT_TOKEN,
  TPR_EXPECT_NEXT_TYPE,
  TPR_MALLOC_ERROR,
  TPR_SYNTAX_ERROR,
  TPR_DONE,

} tp_result;

typedef struct
{
  type_bldr *stack;
  u32 sp;

  lalloc *type_allocator;
} type_parser;

/**
 * T -> p |
 *      struct { i T K } |
 *      union { i T K } |
 *      enum { i I } |
 *      A T
 *
 * A -> V | S
 * K -> \eps | , T K
 * V -> [] V | []
 * S -> [NUM] S | [NUM]
 * I -> \eps | , i I
 *
 * Note:
 * T = TYPE
 * A = ARRAY_PREFIX
 * K = KEY_VALUE_LIST
 * V = VARRAY_BRACKETS
 * S = SARRAY_BRACKETS
 * I = ENUM_KEY_LIST
 */

typedef struct
{

  enum
  {
    SB_WAITING_FOR_LB,
    SB_WAITING_FOR_IDENT,
    SB_WAITING_FOR_TYPE,
    SB_WAITING_FOR_COMMA_OR_RIGHT,
  } state;

  string *keys;
  type *types;
  u32 len;
  u32 cap;

} struct_bldr;

typedef struct
{

  enum
  {
    UB_WAITING_FOR_LB,
    UB_WAITING_FOR_IDENT,
    UB_WAITING_FOR_TYPE,
    UB_WAITING_FOR_COMMA_OR_RIGHT,
  } state;

  string *keys;
  type *types;
  u32 len;
  u32 cap;

} union_bldr;

typedef struct
{

  enum
  {
    EB_WAITING_FOR_LB,
    EB_WAITING_FOR_IDENT,
    EB_WAITING_FOR_COMMA_OR_RIGHT,
  } state;

  string *keys;
  u32 len;
  u32 cap;

} enum_bldr;

typedef struct
{

  enum
  {
    VAB_WAITING_FOR_LEFT_OR_TYPE,
    VAB_WAITING_FOR_RIGHT,
    VAB_WAITING_FOR_TYPE,
  } state;

  u32 rank;

} varray_bldr;

typedef struct
{

  enum
  {
    SAB_WAITING_FOR_LEFT_OR_TYPE,
    SAB_WAITING_FOR_NUMBER,
    SAB_WAITING_FOR_RIGHT,
    SAB_WAITING_FOR_TYPE,
  } state;

  u32 *dims;
  u32 len;
  u32 cap;

} sarray_bldr;

DEFINE_DBG_ASSERT_H (sarray_bldr, sarray_bldr, s);
tp_result sabldr_incr (sarray_bldr *sb, u32 dim);

struct type_bldr_s
{

  enum
  {
    TB_UNKNOWN,
    TB_ARRAY_UNKNOWN,

    TB_STRUCT,
    TB_UNION,
    TB_ENUM,
    TB_VARRAY,
    TB_SARRAY,
    TB_PRIM,

  } state;

  union
  {
    struct_bldr sb;
    union_bldr ub;
    enum_bldr eb;
    varray_bldr vab;
    sarray_bldr sab;
  };

  type ret;
};

static inline type_bldr
tb_create (void)
{
  return (type_bldr){
    .state = TB_UNKNOWN,
  };
}

err_t
tp_create (type_parser *dest, lalloc *type_allocator)
{
  ASSERT (dest);
  lalloc_assert (type_allocator);

  type_bldr *stack = lmalloc (type_allocator, 3 * sizeof *stack);
  if (!stack)
    {
      return ERR_OVERFLOW;
    }

  dest->type_allocator = type_allocator;
  dest->stack = stack;
  dest->sp = 0;

  dest->stack[dest->sp++] = tb_create ();

  return SUCCESS;
}

DEFINE_DBG_ASSERT_I (type_bldr, type_bldr, tb)
{
  ASSERT (tb);
}

//////////////// STRUCT BUILDER
static inline DEFINE_DBG_ASSERT_I (struct_bldr, struct_bldr, s)
{
  ASSERT (s);
  ASSERT (s->keys);
  ASSERT (s->types);
  ASSERT (s->cap > 0);
  ASSERT (s->len <= s->cap);
}

static inline tp_result
stbldr_create (type_bldr *dest, lalloc *alloc)
{
  ASSERT (dest);
  lalloc_assert (alloc);
  ASSERT (dest->state == TB_UNKNOWN);

  dest->sb.keys = lmalloc (alloc, 5 * sizeof *dest->sb.keys);
  if (!dest->sb.keys)
    {
      return TPR_MALLOC_ERROR;
    }
  dest->sb.types = lmalloc (alloc, 5 * sizeof *dest->sb.types);
  if (!dest->sb.types)
    {
      lfree (alloc, dest->sb.keys);
      return TPR_MALLOC_ERROR;
    }

  dest->sb.cap = 5;
  dest->sb.len = 0;
  dest->sb.state = SB_WAITING_FOR_LB;
  dest->state = TB_STRUCT;

  type_bldr_assert (dest);

  return TPR_EXPECT_NEXT_TOKEN;
}

static inline tp_result
sb_handle_waiting_for_lb (type_bldr *sb, token t)
{
  ASSERT (sb->state == TB_STRUCT);
  ASSERT (sb->sb.state == SB_WAITING_FOR_LB);

  if (t.type != TT_LEFT_BRACE)
    {
      return TPR_SYNTAX_ERROR;
    }

  sb->sb.state = SB_WAITING_FOR_IDENT;

  return TPR_EXPECT_NEXT_TOKEN;
}

// Push a string to the end
static inline tp_result
stbldr_push_key (type_bldr *sb, token t, lalloc *alloc)
{
  struct_bldr_assert (&sb->sb);
  lalloc_assert (alloc);
  ASSERT (sb->state == TB_STRUCT);
  ASSERT (sb->sb.state == SB_WAITING_FOR_IDENT);

  if (t.type != TT_IDENTIFIER)
    {
      return TPR_SYNTAX_ERROR;
    }
  string_assert (&t.str);

  // Check for size adjustments
  if (sb->sb.len == sb->sb.cap)
    {
      // Restrict memory requirements to avoid overflow
      ASSERT (can_mul_u64 (sb->sb.cap, 2));
      ASSERT (can_mul_u64 (2 * sb->sb.cap, sizeof (string)));
      u32 ncap = 2 * sb->sb.cap;
      string *keys = lrealloc (alloc, sb->sb.keys, ncap * sizeof *keys);
      if (!keys)
        {
          return TPR_MALLOC_ERROR;
        }
      type *types = lrealloc (alloc, sb->sb.types, ncap * sizeof *types);
      if (!keys)
        {
          lfree (alloc, keys);
          return TPR_MALLOC_ERROR;
        }
      sb->sb.keys = keys;
      sb->sb.types = types;
      sb->sb.cap = ncap;
    }

  sb->sb.keys[sb->sb.len] = t.str; // DONT INC LEN - WAIT FOR TYPE ADD
  sb->sb.state = SB_WAITING_FOR_TYPE;

  return TPR_EXPECT_NEXT_TYPE;
}

static inline tp_result
sb_handle_comma_or_right (type_bldr *sb, token t)
{
  type_bldr_assert (sb);
  ASSERT (sb->state == TB_STRUCT);
  ASSERT (sb->sb.state == SB_WAITING_FOR_COMMA_OR_RIGHT);

  if (t.type == TT_COMMA)
    {
      sb->sb.state = SB_WAITING_FOR_IDENT;
      return TPR_EXPECT_NEXT_TOKEN;
    }
  else if (t.type == TT_RIGHT_BRACE)
    {
      return TPR_DONE;
    }
  return TPR_SYNTAX_ERROR;
}

static inline tp_result
sb_accept_token (type_bldr *tb, token t, lalloc *alloc)
{
  type_bldr_assert (tb);
  lalloc_assert (alloc);
  ASSERT (tb->state == TB_STRUCT);

  switch (tb->sb.state)
    {
    case SB_WAITING_FOR_LB:
      {
        return sb_handle_waiting_for_lb (tb, t);
      }

    case SB_WAITING_FOR_IDENT:
      {
        return stbldr_push_key (tb, t, alloc);
      }

    case SB_WAITING_FOR_COMMA_OR_RIGHT:
      {
        return sb_handle_comma_or_right (tb, t);
      }

    default:
      {
        return TPR_SYNTAX_ERROR;
      }
    }
}

static inline tp_result
stbldr_push_type (type_bldr *sb, type t)
{
  type_bldr_assert (sb);
  type_assert (&t);
  ASSERT (sb->sb.len < sb->sb.cap); // From previous string push
  ASSERT (sb->state == TB_STRUCT);
  ASSERT (sb->sb.state == SB_WAITING_FOR_TYPE);

  sb->sb.types[sb->sb.len++] = t; // NOW YOU CAN INC LEN
  sb->sb.state = SB_WAITING_FOR_COMMA_OR_RIGHT;

  return TPR_EXPECT_NEXT_TOKEN;
}

tp_result
sb_accept_type (type_bldr *sb, type type)
{
  type_bldr_assert (sb);
  ASSERT (sb->state == TB_STRUCT);

  switch (sb->sb.state)
    {
    case SB_WAITING_FOR_TYPE:
      {
        return stbldr_push_type (sb, type);
      }
    default:
      {
        return TPR_SYNTAX_ERROR;
      }
    }
}

tp_result
sb_build (type_bldr *sb, lalloc *alloc)
{
  type_bldr_assert (sb);
  ASSERT (sb->state == TB_STRUCT);
  ASSERT (sb->sb.state == SB_WAITING_FOR_COMMA_OR_RIGHT);

  type *types = sb->sb.types;
  string *strs = sb->sb.keys;

  sb->ret.st = lmalloc (alloc, sizeof *sb->ret.st);
  if (!sb->ret.st)
    {
      return TPR_MALLOC_ERROR;
    }

  // Clip buffers
  if (sb->sb.len < sb->sb.cap)
    {
      types = lrealloc (alloc, types, sb->sb.len * sizeof *types);
      if (!types)
        {
          return TPR_MALLOC_ERROR;
        }

      strs = lrealloc (alloc, strs, sb->sb.len * sizeof *strs);
      if (!strs)
        {
          return TPR_MALLOC_ERROR;
        }
    }

  sb->ret.st->types = types;
  sb->ret.st->keys = strs;
  sb->ret.st->len = sb->sb.len;
  sb->ret.type = T_STRUCT;

  return TPR_DONE;
}

//////////////// UNION BUILDER
static inline DEFINE_DBG_ASSERT_I (union_bldr, union_bldr, u)
{
  ASSERT (u);
  ASSERT (u->keys);
  ASSERT (u->types);
  ASSERT (u->cap > 0);
  ASSERT (u->len <= u->cap);
}

static inline tp_result
unbldr_create (type_bldr *dest, lalloc *alloc)
{
  ASSERT (dest);
  lalloc_assert (alloc);
  ASSERT (dest->state == TB_UNKNOWN);

  dest->ub.keys = lmalloc (alloc, 5 * sizeof *dest->ub.keys);
  if (!dest->ub.keys)
    {
      return TPR_MALLOC_ERROR;
    }

  dest->ub.types = lmalloc (alloc, 5 * sizeof *dest->ub.types);
  if (!dest->ub.types)
    {
      lfree (alloc, dest->ub.keys);
      return TPR_MALLOC_ERROR;
    }

  dest->ub.cap = 5;
  dest->ub.len = 0;
  dest->ub.state = UB_WAITING_FOR_LB;
  dest->state = TB_UNION;

  type_bldr_assert (dest);

  return TPR_EXPECT_NEXT_TOKEN;
}

static inline tp_result
ub_handle_waiting_for_lb (type_bldr *ub, token t)
{
  type_bldr_assert (ub);
  ASSERT (ub->state == TB_UNION);
  ASSERT (ub->ub.state == UB_WAITING_FOR_LB);

  if (t.type != TT_LEFT_BRACE)
    {
      return TPR_SYNTAX_ERROR;
    }

  ub->ub.state = UB_WAITING_FOR_IDENT;

  return TPR_EXPECT_NEXT_TOKEN;
}

// Push a key to the end
static inline tp_result
unbldr_push_key (type_bldr *ub, token t, lalloc *alloc)
{
  type_bldr_assert (ub);
  lalloc_assert (alloc);
  ASSERT (ub->state == TB_UNION);
  ASSERT (ub->ub.state == UB_WAITING_FOR_IDENT);

  if (t.type != TT_IDENTIFIER)
    {
      return TPR_SYNTAX_ERROR;
    }
  string_assert (&t.str);

  // Check for size adjustments
  if (ub->ub.len == ub->ub.cap)
    {
      // Restrict memory requirements to avoid overflow
      ASSERT (can_mul_u64 (ub->ub.cap, 2));
      ASSERT (can_mul_u64 (2 * ub->ub.cap, sizeof (string)));
      u32 ncap = 2 * ub->ub.cap;
      string *keys = lrealloc (alloc, ub->ub.keys, ncap * sizeof (string));
      if (!keys)
        {
          return TPR_MALLOC_ERROR;
        }
      type *types = lrealloc (alloc, ub->ub.types, ncap * sizeof (type));
      if (!keys)
        {
          lfree (alloc, keys);
          return TPR_MALLOC_ERROR;
        }
      ub->ub.keys = keys;
      ub->ub.types = types;
      ub->ub.cap = ncap;
    }

  ub->ub.keys[ub->ub.len] = t.str; // DONT INC LEN - WAIT FOR TYPE ADD
  ub->ub.state = UB_WAITING_FOR_TYPE;

  return TPR_EXPECT_NEXT_TYPE;
}

static inline tp_result
ub_handle_comma_or_right (type_bldr *ub, token t)
{
  type_bldr_assert (ub);
  ASSERT (ub->state == TB_UNION);
  ASSERT (ub->ub.state == UB_WAITING_FOR_COMMA_OR_RIGHT);

  if (t.type == TT_COMMA)
    {
      ub->ub.state = UB_WAITING_FOR_IDENT;
      return TPR_EXPECT_NEXT_TOKEN;
    }
  else if (t.type == TT_RIGHT_BRACE)
    {
      return TPR_DONE;
    }

  return TPR_SYNTAX_ERROR;
}

static inline tp_result
ub_accept_token (type_bldr *ub, token t, lalloc *alloc)
{
  type_bldr_assert (ub);
  lalloc_assert (alloc);
  ASSERT (ub->state == TB_UNION);

  switch (ub->ub.state)
    {

    case UB_WAITING_FOR_LB:
      {
        return ub_handle_waiting_for_lb (ub, t);
      }

    case UB_WAITING_FOR_IDENT:
      {
        return unbldr_push_key (ub, t, alloc);
      }

    case UB_WAITING_FOR_COMMA_OR_RIGHT:
      {
        return ub_handle_comma_or_right (ub, t);
      }

    default:
      {
        return TPR_SYNTAX_ERROR;
      }
    }
}

static inline tp_result
unbldr_push_type (type_bldr *ub, type t)
{
  type_bldr_assert (ub);
  type_assert (&t);
  ASSERT (ub->ub.len < ub->ub.cap);
  ASSERT (ub->state == TB_UNION);
  ASSERT (ub->ub.state == UB_WAITING_FOR_TYPE);

  ub->ub.types[ub->ub.len++] = t; // NOW YOU CAN INC LEN
  ub->ub.state = UB_WAITING_FOR_COMMA_OR_RIGHT;

  return TPR_EXPECT_NEXT_TOKEN;
}

static inline tp_result
ub_accept_type (type_bldr *ub, type type)
{
  type_bldr_assert (ub);
  type_assert (&type);
  ASSERT (ub->state == TB_UNION);

  switch (ub->ub.state)
    {
    case UB_WAITING_FOR_TYPE:
      {
        return unbldr_push_type (ub, type);
      }
    default:
      {
        return TPR_SYNTAX_ERROR;
      }
    }
}

tp_result
ub_build (type_bldr *ub, lalloc *alloc)
{
  type_bldr_assert (ub);
  ASSERT (ub->state == TB_UNION);
  ASSERT (ub->ub.state == UB_WAITING_FOR_COMMA_OR_RIGHT);

  type *types = ub->ub.types;
  string *strs = ub->ub.keys;

  ub->ret.un = lmalloc (alloc, sizeof *ub->ret.un);
  if (!ub->ret.un)
    {
      return TPR_MALLOC_ERROR;
    }

  // Clip buffers
  if (ub->ub.len < ub->ub.cap)
    {
      types = lrealloc (alloc, types, ub->ub.len * sizeof *types);
      if (!types)
        {
          return TPR_MALLOC_ERROR;
        }

      strs = lrealloc (alloc, strs, ub->ub.len * sizeof *strs);
      if (!strs)
        {
          return TPR_MALLOC_ERROR;
        }
    }

  ub->ret.un->types = types;
  ub->ret.un->keys = strs;
  ub->ret.un->len = ub->ub.len;
  ub->ret.type = T_UNION;

  return TPR_DONE;
}

//////////////// ENUM BUILDER
DEFINE_DBG_ASSERT_I (enum_bldr, enum_bldr, s)
{
  ASSERT (s);
  ASSERT (s->keys);
  ASSERT (s->cap > 0);
  ASSERT (s->len <= s->cap);
}

static inline tp_result
enbldr_create (type_bldr *dest, lalloc *alloc)
{
  ASSERT (dest);
  ASSERT (dest->state == TB_UNKNOWN);

  dest->eb.keys = lmalloc (alloc, 5 * sizeof *dest->eb.keys);
  if (!dest->eb.keys)
    {
      return TPR_MALLOC_ERROR;
    }

  dest->eb.cap = 5;
  dest->eb.len = 0;
  dest->eb.state = EB_WAITING_FOR_LB;
  dest->state = TB_ENUM;

  type_bldr_assert (dest);

  return TPR_EXPECT_NEXT_TOKEN;
}

static inline tp_result
eb_handle_waiting_for_lb (type_bldr *eb, token t)
{
  type_bldr_assert (eb);
  ASSERT (eb->state == TB_ENUM);

  if (t.type != TT_LEFT_BRACE)
    {
      return TPR_SYNTAX_ERROR;
    }

  eb->eb.state = EB_WAITING_FOR_IDENT;

  return TPR_EXPECT_NEXT_TOKEN;
}

static inline tp_result
enbldr_push_str (type_bldr *eb, token t, lalloc *alloc)
{
  type_bldr_assert (eb);
  lalloc_assert (alloc);
  ASSERT (eb->state == TB_ENUM);
  ASSERT (eb->eb.state == EB_WAITING_FOR_IDENT);

  if (t.type != TT_IDENTIFIER)
    {
      return TPR_SYNTAX_ERROR;
    }
  string_assert (&t.str);

  // Check for size adjustments
  if (eb->eb.len == eb->eb.cap)
    {
      // Restrict memory requirements to avoid overflow
      ASSERT (can_mul_u64 (eb->eb.cap, 2));
      ASSERT (can_mul_u64 (2 * eb->eb.cap, sizeof (string)));
      u32 ncap = 2 * eb->eb.cap;

      string *keys = lrealloc (alloc, eb->eb.keys, ncap * sizeof (string));
      if (!keys)
        {
          return TPR_MALLOC_ERROR;
        }

      eb->eb.keys = keys;
      eb->eb.cap = ncap;
    }

  eb->eb.keys[eb->eb.len++] = t.str;
  eb->eb.state = EB_WAITING_FOR_COMMA_OR_RIGHT;

  return TPR_EXPECT_NEXT_TOKEN;
}

static inline tp_result
eb_handle_comma_or_right (type_bldr *eb, token t)
{
  type_bldr_assert (eb);
  ASSERT (eb->state == TB_ENUM);
  ASSERT (eb->eb.state == EB_WAITING_FOR_COMMA_OR_RIGHT);

  if (t.type == TT_COMMA)
    {
      eb->eb.state = EB_WAITING_FOR_IDENT;
      return TPR_EXPECT_NEXT_TOKEN;
    }
  else if (t.type == TT_RIGHT_BRACE)
    {
      return TPR_DONE;
    }

  return TPR_SYNTAX_ERROR;
}

static inline tp_result
eb_accept_token (type_bldr *eb, token t, lalloc *alloc)
{
  type_bldr_assert (eb);
  lalloc_assert (alloc);
  ASSERT (eb->state == TB_ENUM);

  switch (eb->eb.state)
    {
    case EB_WAITING_FOR_LB:
      {
        return eb_handle_waiting_for_lb (eb, t);
      }

    case EB_WAITING_FOR_IDENT:
      {
        return enbldr_push_str (eb, t, alloc);
      }

    case EB_WAITING_FOR_COMMA_OR_RIGHT:
      {
        return eb_handle_comma_or_right (eb, t);
      }

    default:
      {
        return TPR_SYNTAX_ERROR;
      }
    }
}

tp_result
eb_build (type_bldr *eb, lalloc *alloc)
{
  type_bldr_assert (eb);
  ASSERT (eb->state == TB_STRUCT);
  ASSERT (eb->eb.state == EB_WAITING_FOR_COMMA_OR_RIGHT);

  string *strs = eb->eb.keys;

  eb->ret.en = lmalloc (alloc, sizeof *eb->ret.en);
  if (!eb->ret.en)
    {
      return TPR_MALLOC_ERROR;
    }

  // Clip buffers
  if (eb->eb.len < eb->eb.cap)
    {
      strs = lrealloc (alloc, strs, eb->eb.len * sizeof *strs);
      if (!strs)
        {
          return TPR_MALLOC_ERROR;
        }
    }

  eb->ret.en->keys = strs;
  eb->ret.en->len = eb->eb.len;
  eb->ret.type = T_UNION;

  return TPR_DONE;
}

//////////////// VARRAY BUILDER
static inline DEFINE_DBG_ASSERT_I (varray_bldr, varray_bldr, v)
{
  ASSERT (v);
  ASSERT (v->rank > 0);
}

tp_result
varray_bldr_create (varray_bldr *dest)
{
  ASSERT (dest);

  dest->rank = 1;
  dest->state = VAB_WAITING_FOR_LEFT_OR_TYPE;

  varray_bldr_assert (dest);

  return TPR_EXPECT_NEXT_TOKEN;
}

/**
static inline tp_result
varray_bldr_incr (varray_bldr *sb)
{
  varray_bldr_assert (sb);
  ASSERT (sb->state == VAB_WAITING_FOR_RIGHT);
  sb->rank++;
  sb->state = VAB_WAITING_FOR_LEFT_OR_TYPE;
  return TPR_EXPECT_NEXT_TOKEN;
}
*/

//////////////// SARRAY BUILDER
DEFINE_DBG_ASSERT_I (sarray_bldr, sarray_bldr, s)
{
  ASSERT (s);
  ASSERT (s->dims);
  ASSERT (s->len > 0);
  ASSERT (s->cap > 0);
  ASSERT (s->len <= s->len);

  for (u32 i = 0; i < s->len; ++i)
    {
      ASSERT (s->dims[i] > 0);
    }
}

tp_result
sarray_bldr_create (sarray_bldr *dest, u32 dim, lalloc *alloc)
{
  ASSERT (dest);
  lalloc_assert (alloc);

  dest->dims = lmalloc (alloc, 5 * sizeof *dest->dims);
  if (!dest->dims)
    {
      return TPR_MALLOC_ERROR;
    }
  dest->cap = 5;
  dest->len = 0;
  dest->dims[dest->len++] = dim;
  dest->state = SAB_WAITING_FOR_RIGHT;

  sarray_bldr_assert (dest);

  return TPR_EXPECT_NEXT_TOKEN;
}

tp_result
sarray_bldr_incr (type_bldr *sb, token t, lalloc *alloc)
{
  type_bldr_assert (sb);
  lalloc_assert (alloc);
  ASSERT (sb->state == TB_SARRAY);
  ASSERT (sb->sab.state == SAB_WAITING_FOR_NUMBER);

  if (t.type != TT_INTEGER)
    {
      return TPR_SYNTAX_ERROR;
    }
  if (t.integer <= 0)
    {
      return TPR_SYNTAX_ERROR;
    }
  // TODO - maximum dimension size?

  // Check for size adjustments
  if (sb->sab.len == sb->sab.cap)
    {
      // Restrict memory requirements to avoid overflow
      ASSERT (can_mul_u64 (sb->sab.cap, 2));
      ASSERT (can_mul_u64 (2 * sb->sab.cap, sizeof (string)));
      u32 ncap = 2 * sb->sab.cap;

      u32 *dims = lrealloc (alloc, sb->sab.dims, ncap * sizeof (string));
      if (!dims)
        {
          return TPR_MALLOC_ERROR;
        }

      sb->sab.dims = dims;
      sb->sab.cap = ncap;
    }

  sb->sab.dims[sb->sab.len++] = (u32)t.integer;
  sb->sab.state = SAB_WAITING_FOR_RIGHT;

  return TPR_EXPECT_NEXT_TOKEN;
}

static inline tp_result
prim_create (type_bldr *tb, prim_t p)
{
  type_bldr_assert (tb);
  ASSERT (tb->state == TB_UNKNOWN);

  tb->ret.p = p;
  tb->ret.type = T_PRIM;
  tb->state = TB_PRIM;

  return TPR_DONE;
}

tp_result
tb_accept_token (type_bldr *tb, token t, lalloc *alloc)
{
  type_bldr_assert (tb);
  lalloc_assert (alloc);

  switch (tb->state)
    {
    case TB_UNKNOWN:
      {
        switch (t.type)
          {
          case TT_STRUCT:
            {
              return stbldr_create (tb, alloc);
            }
          case TT_UNION:
            {
              return unbldr_create (tb, alloc);
            }
          case TT_ENUM:
            {
              return enbldr_create (tb, alloc);
            }
          case TT_LEFT_BRACKET:
            {
              // TODO
              ASSERT (0);
              return 0;
            }
          case TT_PRIM:
            {
              return prim_create (tb, t.prim);
            }
          default:
            {
              return TPR_SYNTAX_ERROR;
            }
          }
      }
    case TB_ARRAY_UNKNOWN:
      {
        ASSERT (0);
        return 0;
      }
    case TB_STRUCT:
      {
        return sb_accept_token (tb, t, alloc);
      }
    case TB_UNION:
      {
        return ub_accept_token (tb, t, alloc);
      }
    case TB_ENUM:
      {
        return eb_accept_token (tb, t, alloc);
      }
    case TB_SARRAY:
      {
        ASSERT (0);
        return 0;
      }
    case TB_VARRAY:
      {
        ASSERT (0);
        return 0;
      }
    default:
      {
        return TPR_SYNTAX_ERROR;
      }
    }
  ASSERT (0);
  return 0;
}

tp_result
tb_accept_type (type_bldr *tb, type t)
{
  type_bldr_assert (tb);
  switch (tb->state)
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
tb_build (type_bldr *tb, lalloc *alloc)
{
  type_bldr_assert (tb);

  switch (tb->state)
    {
    case TB_STRUCT:
      {
        return sb_build (tb, alloc);
      }
    case TB_UNION:
      {
        return ub_build (tb, alloc);
      }
    case TB_ENUM:
      {
        return eb_build (tb, alloc);
      }
    case TB_PRIM:
      {
        return TPR_DONE;
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
  tp_result ret = tb_accept_token (
      &tp->stack[tp->sp - 1], t, tp->type_allocator);

  // We expect a new type
  // Push a type builder to
  // the top of the stack
  if (ret == TPR_EXPECT_NEXT_TYPE)
    {
      type_bldr next;
      next.state = TB_UNKNOWN;
      tp->stack[tp->sp++] = next;
      return TPR_EXPECT_NEXT_TOKEN;
    }

  // Otherwise, the acceptor is done
  // and can be reduced with the previous
  // builders
  //
  // sp == 1 means we're at the base
  while (tp->sp > 1 && ret == TPR_DONE)
    {
      // Pop the top off the stack
      type_bldr top = tp->stack[--tp->sp];
      if ((ret = tb_build (&top, tp->type_allocator)) != TPR_DONE)
        {
          return ret;
        }

      // Merge it with the previous
      ret = tb_accept_type (&tp->stack[tp->sp - 1], top.ret);
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
  type_parser tp;

  scanner_create (&s, &input, &output, &alloc);
  test_fail_if (tp_create (&tp, &alloc));

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

  tb_build (&tp.stack[0], tp.type_allocator);
}

tp_result tp_feed_token (type_parser *tp, token t);
