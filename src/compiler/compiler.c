#include "compiler/compiler.h"

#include "core/dev/assert.h"    // DEFINE_DBG_ASSERT_I
#include "core/dev/testing.h"   // TEST
#include "core/ds/cbuffer.h"    // cbuffer
#include "core/errors/error.h"  // err_t
#include "core/intf/logging.h"  // TODO
#include "core/mm/lalloc.h"     // lalloc
#include "core/utils/macros.h"  // is_alpha
#include "core/utils/numbers.h" // parse_i32_expect

#include "compiler/parser.h" // parser
#include "compiler/tokens.h" // token

#include "numstore/query/queries/create.h" // TODO
#include "numstore/query/query.h"          // query
#include "numstore/query/query_provider.h" // TODO

typedef struct
{
  enum
  {
    SS_START,   // Start here and finish here
    SS_IDENT,   // Parsing an identifier or magic string
    SS_STRING,  // Parsing a "string"
    SS_NUMBER,  // Parsing the integer part of an int or float
    SS_DECIMAL, // Parsing the right half of a dec
    SS_ERR,     // Rewinding to next TT_SEMICOLON
  } state;

  union
  {
    // SS_IDENT, SS_STRING, SS_INTEGER, SS_DECIMAL
    struct
    {
      u32 slen;
      char str[512];
    };
  };
} compiler_state;

struct compiler_s
{
  compiler_state state;

  // Input and outputs
  cbuffer input;
  cbuffer output;

  char _input[10];
  query _output[10];

  /////////// PARSER
  // Allocator for temporary variables in parser
  lalloc parser_work;
  u8 _parser_work[2048];
  parser_result parser;

  error e;
  query_provider *qp;
};

DEFINE_DBG_ASSERT_I (compiler, compiler, s)
{
  ASSERT (s);
}

static const char *TAG = "Compiler";

////////////////////////////// Premade magic strings

typedef struct
{
  const string token;
  token t;
} prim_token;

typedef struct
{
  string token;
  token t;
} magic_token;

#define STR_LIT(s)        \
  {                       \
    (sizeof (s) - 1), (s) \
  }

//// TODO - implement a trie for faster string searches
/**
 * Seperate because you need to create
 * a new query for each one
 */
static const magic_token op_codes[] = {
  // JSON commands
  { .token = STR_LIT ("create"), .t = { .type = TT_CREATE } },
  { .token = STR_LIT ("delete"), .t = { .type = TT_DELETE } },
  { .token = STR_LIT ("append"), .t = { .type = TT_APPEND } },
  { .token = STR_LIT ("insert"), .t = { .type = TT_INSERT } },
  { .token = STR_LIT ("update"), .t = { .type = TT_UPDATE } },
  { .token = STR_LIT ("read"), .t = { .type = TT_READ } },
  { .token = STR_LIT ("take"), .t = { .type = TT_TAKE } },

  // binary commands
  { .token = STR_LIT ("bcreate"), .t = { .type = TT_BCREATE } },
  { .token = STR_LIT ("bdelete"), .t = { .type = TT_BDELETE } },
  { .token = STR_LIT ("bappend"), .t = { .type = TT_BAPPEND } },
  { .token = STR_LIT ("binsert"), .t = { .type = TT_BINSERT } },
  { .token = STR_LIT ("bupdate"), .t = { .type = TT_BUPDATE } },
  { .token = STR_LIT ("bread"), .t = { .type = TT_BREAD } },
  { .token = STR_LIT ("btake"), .t = { .type = TT_BTAKE } },
};

static const magic_token magic_tokens[] = {
  // types
  { .token = STR_LIT ("struct"), .t = { .type = TT_STRUCT } },
  { .token = STR_LIT ("union"), .t = { .type = TT_UNION } },
  { .token = STR_LIT ("enum"), .t = { .type = TT_ENUM } },
  { .token = STR_LIT ("u8"), .t = { .type = TT_PRIM, .prim = U8 } },
  { .token = STR_LIT ("u16"), .t = { .type = TT_PRIM, .prim = U16 } },
  { .token = STR_LIT ("u32"), .t = { .type = TT_PRIM, .prim = U32 } },
  { .token = STR_LIT ("u64"), .t = { .type = TT_PRIM, .prim = U64 } },
  { .token = STR_LIT ("i8"), .t = { .type = TT_PRIM, .prim = I8 } },
  { .token = STR_LIT ("i16"), .t = { .type = TT_PRIM, .prim = I16 } },
  { .token = STR_LIT ("i32"), .t = { .type = TT_PRIM, .prim = I32 } },
  { .token = STR_LIT ("i64"), .t = { .type = TT_PRIM, .prim = I64 } },
  { .token = STR_LIT ("f16"), .t = { .type = TT_PRIM, .prim = F16 } },
  { .token = STR_LIT ("f32"), .t = { .type = TT_PRIM, .prim = F32 } },
  { .token = STR_LIT ("f64"), .t = { .type = TT_PRIM, .prim = F64 } },
  { .token = STR_LIT ("f128"), .t = { .type = TT_PRIM, .prim = F128 } },
  { .token = STR_LIT ("cf32"), .t = { .type = TT_PRIM, .prim = CF32 } },
  { .token = STR_LIT ("cf64"), .t = { .type = TT_PRIM, .prim = CF64 } },
  { .token = STR_LIT ("cf128"), .t = { .type = TT_PRIM, .prim = CF128 } },
  { .token = STR_LIT ("cf256"), .t = { .type = TT_PRIM, .prim = CF256 } },
  { .token = STR_LIT ("ci16"), .t = { .type = TT_PRIM, .prim = CI16 } },
  { .token = STR_LIT ("ci32"), .t = { .type = TT_PRIM, .prim = CI32 } },
  { .token = STR_LIT ("ci64"), .t = { .type = TT_PRIM, .prim = CI64 } },
  { .token = STR_LIT ("ci128"), .t = { .type = TT_PRIM, .prim = CI128 } },
  { .token = STR_LIT ("cu16"), .t = { .type = TT_PRIM, .prim = CU16 } },
  { .token = STR_LIT ("cu32"), .t = { .type = TT_PRIM, .prim = CU32 } },
  { .token = STR_LIT ("cu64"), .t = { .type = TT_PRIM, .prim = CU64 } },
  { .token = STR_LIT ("cu128"), .t = { .type = TT_PRIM, .prim = CU128 } },

  { .token = STR_LIT ("true"), .t = { .type = TT_TRUE } },
  { .token = STR_LIT ("false"), .t = { .type = TT_FALSE } },
};

////////////////////////////// State Machine Functions
/// Forward Declaration

/**
 * Usage:
 * steady_state_execute_max_STATE is the function to call during
 * steady state of a token scanning step. Some tokens
 * have special behavior on the first character (e.g.
 * identifiers can start with alpha, then later on
 * can be alpha numeric)
 *
 * When steady state notices that it's done, it will
 * call [compiler_process_token], which resets
 * to the SS_START state (and maybe writes the token)
 *
 * Then it's done. ss_transition returns and is not
 * recursive (to save the parser_stack), so you need to continue
 * calling ss_execute until your input buffer is empty
 */
// Picks which function below to execute
static err_t steady_state_execute_max (compiler *s);
static err_t steady_state_execute_max_start (compiler *s);
static err_t steady_state_execute_max_ident (compiler *s);
static err_t steady_state_execute_max_string (compiler *s);
static err_t steady_state_execute_max_number (compiler *s);
static err_t steady_state_execute_max_dec (compiler *s);
static void steady_state_execute_max_err (compiler *s);

/////////////////////////////////// UTILS
/// Some simple tools to help scanning like special
/// advance functions etc.

#define compiler_err_t_wrap(expr, c)      \
  do                                      \
    {                                     \
      err_t __ret = (err_t)expr;          \
      if (__ret < SUCCESS)                \
        {                                 \
          if (c->state.state != SS_ERR)   \
            {                             \
              c->state.state = SS_ERR;    \
              compiler_process_error (c); \
            }                             \
          return __ret;                   \
        }                                 \
    }                                     \
  while (0)

#define compiler_err_t_wrap_null(expr, c) \
  do                                      \
    {                                     \
      err_t __ret = (err_t)expr;          \
      if (__ret < SUCCESS)                \
        {                                 \
          if (c->state.state != SS_ERR)   \
            {                             \
              c->state.state = SS_ERR;    \
              compiler_process_error (c); \
            }                             \
          return NULL;                    \
        }                                 \
    }                                     \
  while (0)

#define compiler_err_t_pass(expr, c)                         \
  do                                                         \
    {                                                        \
      err_t __ret = (err_t)expr;                             \
      i_log_trace ("%s: %s\n", #expr, err_t_to_str (__ret)); \
      if (__ret < SUCCESS)                                   \
        {                                                    \
          if (c->state.state != SS_ERR)                      \
            {                                                \
              c->state.state = SS_ERR;                       \
              compiler_process_error (c);                    \
            }                                                \
        }                                                    \
    }                                                        \
  while (0)

const char *
compiler_state_to_str (compiler_state state)
{
  switch (state.state)
    {
    case SS_START:
      {
        return "SS_START";
      }
    case SS_IDENT:
      {
        return "SS_IDENT";
      }
    case SS_NUMBER:
      {
        return "SS_NUMBER";
      }
    case SS_STRING:
      {
        return "SS_STRING";
      }
    case SS_DECIMAL:
      {
        return "SS_DECIMAL";
      }
    case SS_ERR:
      {
        return "SS_ERR";
      }
    }
  UNREACHABLE ();
}

static inline void
compiler_write_result (compiler *s, query res)
{
  ASSERT (cbuffer_avail (&s->output) >= sizeof (res));
  u32 ret = cbuffer_write (&res, sizeof res, 1, &s->output);
  ASSERT (ret == 1);
}

static inline void
compiler_process_error (compiler *s)
{
  compiler_assert (s);

  ASSERT (s->state.state == SS_ERR);

  // Write to output buffer
  compiler_write_result (s, query_error_create (s->e));

  s->e = error_create (NULL);
}

/**
 * Pushes [next] onto the internal string
 * that compiler holds
 *
 * Errors:
 *    - ERR_NOMEM - internal parser_stack overflows
 */
static inline err_t
compiler_push_char (compiler *s, char next)
{
  compiler_assert (s);

  if (s->state.slen == 512)
    {
      compiler_err_t_wrap (
          error_causef (
              &s->e, ERR_NOMEM,
              "%s: internal buffer overflow",
              TAG),
          s);
      UNREACHABLE ();
    }

  s->state.str[s->state.slen++] = next;

  return SUCCESS;
}

/**
 * Advances forward one char
 * ASSERTS that it can actually do that
 *
 * Used if you call peek and know what the
 * next char is, so you know you can advance
 */
static inline void
compiler_advance_expect (compiler *s)
{
  compiler_assert (s);
  u8 next;
  bool more = cbuffer_dequeue (&next, &s->input);
  ASSERT (more);
}

/**
 * Advances forward while also copying the
 * result char into the internal string
 * ASSERTS that it can actually do that
 *
 * Errors:
 *    - ERR_NOMEM - Can't push char to the internal string
 */
static inline err_t
compiler_cpy_advance_expect (compiler *s)
{
  compiler_assert (s);

  // Dequeue and copy
  u8 next;
  bool more = cbuffer_dequeue (&next, &s->input);
  ASSERT (more);

  compiler_err_t_wrap (compiler_push_char (s, next), s);

  return SUCCESS;
}

/**
 * Peeks forward one char - returns
 * true if success, false otherwise
 */
static inline bool
compiler_peek (u8 *dest, compiler *s)
{
  compiler_assert (s);
  return cbuffer_peek_dequeue (dest, &s->input);
}

static inline err_t
compiler_xfer_str_onto_parser_alloc (string *dest, compiler *s)
{
  compiler_assert (s);
  ASSERT (s->state.slen > 0);
  char *ret = lmalloc (&s->parser_work, 1, s->state.slen);
  if (ret == NULL)
    {
      compiler_err_t_wrap (
          error_causef (
              &s->e, ERR_NOMEM,
              "%s: internal buffer overflow",
              TAG),
          s);
      UNREACHABLE ();
    }
  i_memcpy (ret, s->state.str, s->state.slen);
  *dest = (string){
    .data = ret,
    .len = s->state.slen,
  };
  s->state.slen = 0;
  return SUCCESS;
}

/**
 * Skips over chars until it finds non white space
 */
static inline void
consume_whitespace (compiler *s)
{
  compiler_assert (s);
  u8 c;

  while (compiler_peek (&c, s))
    {
      switch (c)
        {
        case ' ':
        case '\t':
        case '\r':
        case '\n':
          compiler_advance_expect (s);
          break;
        default:
          return;
        }
    }
}

static inline err_t
compiler_process_token (compiler *s, token t)
{
  compiler_assert (s);

  // Parse this token

  compiler_err_t_wrap (parser_parse (&s->parser, t, &s->e), s);

  // Check parser state
  if (s->parser.ready)
    {
      compiler_err_t_wrap (parser_parse (&s->parser, quick_tok (0), &s->e), s);

      // Consume query
      query next = parser_consume (&s->parser);

      // Reset allocator
      lalloc_reset (&s->parser_work);

      // Execute
      i_log_query (next);

      // Write to output buffer
      compiler_write_result (s, next);
    }

  s->state.state = SS_START;

  return SUCCESS;
}

////////////////////////////// State Machine Functions
static err_t
steady_state_execute_max (compiler *s)
{
  compiler_assert (s);

  // Pick up where you left off
  switch (s->state.state)
    {
    case SS_START:
      {
        return steady_state_execute_max_start (s);
      }
    case SS_IDENT:
      {
        return steady_state_execute_max_ident (s);
      }
    case SS_NUMBER:
      {
        return steady_state_execute_max_number (s);
      }
    case SS_DECIMAL:
      {
        return steady_state_execute_max_dec (s);
      }
    case SS_STRING:
      {
        return steady_state_execute_max_string (s);
      }
    case SS_ERR:
      {
        steady_state_execute_max_err (s);
        return SUCCESS;
      }
    default:
      {
        UNREACHABLE ();
      }
    }
}

static err_t
process_string_or_ident (compiler *s, token_t type)
{
  compiler_assert (s);
  ASSERT (type == TT_STRING || type == TT_IDENTIFIER);

  string literal;

  compiler_err_t_wrap (compiler_xfer_str_onto_parser_alloc (&literal, s), s);

  // Process the token
  token t = (token){
    .type = type,
    .str = literal,
  };

  return compiler_process_token (s, t);
}

/**
 * Processes a string.
 * It could be:
 *  1. An Ident (any string like token that isn't well known/magic)
 *  2. Magic word (struct, union etc)
 *  3. String ("foobar")
 */
static err_t
process_maybe_ident (compiler *s)
{
  compiler_assert (s);

  ASSERT (s->state.slen > 0);

  string literal = (string){
    .data = s->state.str,
    .len = s->state.slen,
  };

  // Check for magic tokens
  for (u32 i = 0; i < arrlen (magic_tokens); ++i)
    {
      if (string_equal (literal, magic_tokens[i].token))
        {
          // reset (literal is useless)
          s->state.slen = 0;
          return compiler_process_token (s, magic_tokens[i].t);
        }
    }

  // Check for op codes
  for (u32 i = 0; i < arrlen (op_codes); ++i)
    {
      if (string_equal (literal, op_codes[i].token))
        {
          token t = op_codes[i].t;

          // reset (literal is useless)
          s->state.slen = 0;

          // Allocate query space for downstream
          query q;
          compiler_err_t_wrap (query_provider_get (s->qp, &q, tt_to_qt (t.type), &s->e), s);

          // Initialize the parser
          compiler_err_t_wrap (parser_create (&s->parser, &s->parser_work, q.qalloc, &s->e), s);

          t.q = q;

          return compiler_process_token (s, t);
        }
    }

  // Otherwise it's an TT_IDENTIFIER
  return process_string_or_ident (s, TT_IDENTIFIER);
}

static err_t
steady_state_execute_max_ident (compiler *s)
{
  compiler_assert (s);
  ASSERT (s->state.state == SS_IDENT);

  u8 c;

  while (compiler_peek (&c, s))
    {
      if (!is_alpha_num (c))
        {
          return process_maybe_ident (s);
        }

      compiler_err_t_wrap (compiler_cpy_advance_expect (s), s);
    }

  return SUCCESS; // no more chars to consume
}

static err_t
steady_state_execute_max_string (compiler *s)
{
  compiler_assert (s);
  ASSERT (s->state.state == SS_STRING);

  u8 c;

  while (compiler_peek (&c, s))
    {
      if (c == '"')
        {
          compiler_advance_expect (s); // Skip over (last) quote
          return process_string_or_ident (s, TT_STRING);
        }

      compiler_err_t_wrap (compiler_cpy_advance_expect (s), s);
    }

  return SUCCESS;
}

static inline err_t
process_dec (compiler *s)
{
  compiler_assert (s);

  const string literal = (string){
    .data = s->state.str,
    .len = s->state.slen,
  };
  s->state.slen = 0;

  // Parse the float
  f32 dest;
  compiler_err_t_wrap (parse_f32_expect (&dest, literal, &s->e), s);

  compiler_err_t_wrap (compiler_process_token (s, tt_float (dest)), s);

  return SUCCESS;
}

static err_t
steady_state_execute_max_dec (compiler *s)
{
  compiler_assert (s);
  ASSERT (s->state.state == SS_DECIMAL);

  u8 c;
  while (compiler_peek (&c, s))
    {
      if (!is_num (c))
        {
          return process_dec (s);
        }
      compiler_err_t_wrap (compiler_cpy_advance_expect (s), s);
    }

  return SUCCESS;
}

static void
steady_state_execute_max_err (compiler *s)
{
  compiler_assert (s);
  ASSERT (s->state.state == SS_ERR);

  u8 c;
  while (compiler_peek (&c, s))
    {
      if (c == ';')
        {
          compiler_advance_expect (s); // Skip over the ;
          s->state.state = SS_START;
          return;
        }
    }
}

static inline err_t
process_int (compiler *s)
{
  compiler_assert (s);

  const string literal = (string){
    .data = s->state.str,
    .len = s->state.slen,
  };
  s->state.slen = 0;

  // Parse the int
  i32 dest;
  compiler_err_t_wrap (parse_i32_expect (&dest, literal, &s->e), s);

  compiler_err_t_wrap (compiler_process_token (s, tt_integer (dest)), s);

  return SUCCESS;
}

static err_t
steady_state_execute_max_number (compiler *s)
{
  compiler_assert (s);
  ASSERT (s->state.state == SS_NUMBER);

  u8 c;

  while (compiler_peek (&c, s))
    {
      if (!is_num (c))
        {
          if (c == '.')
            {
              s->state.state = SS_DECIMAL;
              compiler_err_t_wrap (compiler_cpy_advance_expect (s), s);
              return steady_state_execute_max_dec (s);
            }
          else
            {
              return process_int (s);
            }
        }
      compiler_err_t_wrap (compiler_cpy_advance_expect (s), s);
    }

  return SUCCESS;
}

static err_t
steady_state_execute_max_start (compiler *s)
{
  compiler_assert (s);
  ASSERT (s->state.state == SS_START);

  consume_whitespace (s);

  u8 next;
  if (!compiler_peek (&next, s))
    {
      // no more tokens (could happen if we have a ton of whitespace)
      return SUCCESS;
    }

  switch (next)
    {
    /**
     * In all the 1 tokens, process then stay in SS_START for next
     */
    case ';':
      {
        compiler_advance_expect (s);
        compiler_err_t_wrap (compiler_process_token (s, quick_tok (TT_SEMICOLON)), s);
        break;
      }
    case ':':
      {
        compiler_advance_expect (s);
        compiler_err_t_wrap (compiler_process_token (s, quick_tok (TT_COLON)), s);
        break;
      }
    case '[':
      {
        compiler_advance_expect (s);
        compiler_err_t_wrap (compiler_process_token (s, quick_tok (TT_LEFT_BRACKET)), s);
        break;
      }
    case ']':
      {
        compiler_advance_expect (s);
        compiler_err_t_wrap (compiler_process_token (s, quick_tok (TT_RIGHT_BRACKET)), s);
        break;
      }
    case '{':
      {
        compiler_advance_expect (s);
        compiler_err_t_wrap (compiler_process_token (s, quick_tok (TT_LEFT_BRACE)), s);
        break;
      }
    case '}':
      {
        compiler_advance_expect (s);
        compiler_err_t_wrap (compiler_process_token (s, quick_tok (TT_RIGHT_BRACE)), s);
        break;
      }
    case '(':
      {
        compiler_advance_expect (s);
        compiler_err_t_wrap (compiler_process_token (s, quick_tok (TT_LEFT_PAREN)), s);
        break;
      }
    case ')':
      {
        compiler_advance_expect (s);
        compiler_err_t_wrap (compiler_process_token (s, quick_tok (TT_RIGHT_PAREN)), s);
        break;
      }
    case ',':
      {
        compiler_advance_expect (s);
        compiler_err_t_wrap (compiler_process_token (s, quick_tok (TT_COMMA)), s);
        break;
      }
    case '"':
      {
        s->state.slen = 0;
        compiler_advance_expect (s); // Skip the '"'
        s->state.state = SS_STRING;
        break;
      }
    default:
      {
        if (is_alpha (next))
          {
            /*
             * Due to maximal munch all magic strings
             * are treated as ident - then checked at the end
             *
             * Advance forward once - because strings can start with
             * alpha, then steady state processes alpha numeric
             */
            s->state.slen = 0;
            compiler_err_t_wrap (compiler_cpy_advance_expect (s), s);
            s->state.state = SS_IDENT;
            break;
          }
        else if (is_num (next) || next == '+' || next == '-')
          {
            /**
             * Advance forward once because steady state doesn't
             * account for + or -
             */
            s->state.slen = 0;
            compiler_err_t_wrap (compiler_cpy_advance_expect (s), s);
            s->state.state = SS_NUMBER;
            break;
          }
        else
          {
            compiler_advance_expect (s);
            compiler_err_t_wrap (
                error_causef (
                    &s->e, ERR_SYNTAX,
                    "%s: Unexpected char: %c",
                    TAG, next),
                s);
            UNREACHABLE ();
          }
      }
    }
  return SUCCESS;
}

////////////////////////////// Main Functions

compiler *
compiler_create (query_provider *qp, error *e)
{
  // Allocate compiler
  compiler *ret = i_malloc (1, sizeof *ret);
  if (ret == NULL)
    {
      error_causef (e, ERR_NOMEM, "%s Failed to allocate compiler", TAG);
      return NULL;
    }

  *ret = (compiler){
    .state = {
        .state = SS_START,
        .slen = 0,
    },
    .input = cbuffer_create_from (ret->_input),
    .output = cbuffer_create_from (ret->_output),

    .parser_work = lalloc_create_from (ret->_parser_work),
    // parser gets initialized on op codes

    .qp = qp,
    .e = error_create (NULL),
  };

  compiler_assert (ret);

  return ret;
}

TEST (compiler_create)
{
  // Create Stuff
  error err = error_create (NULL);
  query_provider *qp = query_provider_create (&err);
  test_fail_if_null (qp);
  compiler *c = compiler_create (qp, &err);
  test_fail_if_null (c);

  // Test
  test_assert_int_equal (c->state.state, SS_START);
  test_assert_int_equal (cbuffer_len (&c->input), 0);
  test_assert_int_equal (cbuffer_len (&c->output), 0);

  // Cleanup
  compiler_free (c);
  query_provider_free (qp);
}

void
compiler_free (compiler *c)
{
  compiler_assert (c);
  i_free (c);
}

cbuffer *
compiler_get_input (compiler *c)
{
  compiler_assert (c);
  return &c->input;
}

cbuffer *
compiler_get_output (compiler *c)
{
  compiler_assert (c);
  return &c->output;
}

void
compiler_execute (compiler *s)
{
  compiler_assert (s);

  while (true)
    {
      /**
       * Block on downstream
       */
      if (cbuffer_avail (&s->output) < sizeof (token))
        {
          return;
        }

      /**
       * Block on upstream
       */
      if (cbuffer_len (&s->input) == 0)
        {
          return;
        }

      compiler_err_t_pass (steady_state_execute_max (s), s);
    }
}

/**
#ifndef NTEST
static inline void
compile_expect (const char *src, query expect)
{
  error err = error_create (NULL);
  query_provider *qp = query_provider_create (&err);
  test_fail_if_null (qp);

  compiler *c = compiler_create (qp, &err);
  test_fail_if_null (c);

  const char *p = src;
  u32 rem = i_unsafe_strlen (src);
  u32 safety = 0;

  while (safety++ < 100)
    {
      // Write
      if (rem > 0)
        {
          u32 wrote = cbuffer_write ((void *)p, rem, 1, compiler_get_input (c));
          p += wrote;
          rem -= wrote;
        }

      // Execute
      compiler_execute (c);

      // Read
      if (cbuffer_len (compiler_get_output (c)) >= sizeof (compiler_result))
        {
          compiler_result res;
          u32 got = cbuffer_read (&res, sizeof res, 1, compiler_get_output (c));
          test_assert_int_equal (got, 1);
          test_assert_int_equal (res.ok, true);
          test_assert_int_equal (res.query.type, expect.type);
          return;
        }
    }

  test_fail_if (true);
  return;
}

#endif

TEST (compiler)
{
  create_query c = {
    .type = (type){
        .p = U32,
        .type = T_PRIM,
    },
    .vname = unsafe_cstrfrom ("foo"),
  };
  compile_expect ("create foo u32;", (query){ .create = &c, .type = QT_CREATE });
}
*/
