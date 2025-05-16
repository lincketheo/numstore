#include "compiler/scanner.h"

#include "ast/query/qspace_provider.h"
#include "compiler/tokens.h" // token

#include "dev/assert.h" // DEFINE_DBG_ASSERT_I
#include "ds/cbuffer.h" // cbuffer
#include "ds/strings.h"
#include "errors/error.h"  // err_t
#include "intf/stdlib.h"   // i_memcpy
#include "utils/macros.h"  // is_alpha
#include "utils/numbers.h" // parse_i32_expect

DEFINE_DBG_ASSERT_I (scanner, scanner, s)
{
  ASSERT (s);
  ASSERT (s->slen <= 512);
}

static const char *TAG = "Scanner";

////////////////////////////// Premade magic strings
//// TODO - implement a trie for faster string searches

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
};

////////////////////////////// State Machine Functions
/// Forward Declaration

/**
 * Usage:
 * steady_state_STATE is the function to call during
 * steady state of a token scanning step. Some tokens
 * have special behavior on the first character (e.g.
 * identifiers can start with alpha, then later on
 * can be alpha numeric)
 *
 * When steady state notices that it's done, it will
 * call [scanner_process_token_expect], which resets
 * to the SS_START state (and maybe writes the token)
 *
 * Then it's done. ss_transition returns and is not
 * recursive (to save the stack), so you need to continue
 * calling ss_execute until your input buffer is empty
 */
// Picks which function below to execute
static err_t steady_state_execute (scanner *s, error *e);
static err_t steady_state_start (scanner *s, error *e);
static err_t steady_state_ident (scanner *s, error *e);
static err_t steady_state_string (scanner *s, error *e);
static err_t steady_state_number (scanner *s, error *e);
static err_t steady_state_dec (scanner *s, error *e);

/////////////////////////////////// UTILS
/// Some simple tools to help scanning like special
/// advance functions etc.

const char *
scanner_state_to_str (scanner_state state)
{
  switch (state)
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
    }
  UNREACHABLE ();
}

/**
 * Pushes [next] onto the internal string
 * that scanner holds
 *
 * Errors:
 *    - ERR_NOMEM - internal stack overflows
 */
static inline err_t
scanner_push_char_dcur (scanner *s, char next, error *e)
{
  scanner_assert (s);

  if (s->slen == 512)
    {
      return error_causef (
          e, ERR_NOMEM,
          "%s: internal buffer overflow",
          TAG);
    }

  s->str[s->slen++] = next;

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
scanner_advance_expect (scanner *s)
{
  scanner_assert (s);
  u8 next;
  bool more = cbuffer_dequeue (&next, s->chars_input);
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
scanner_cpy_advance_expect (scanner *s, error *e)
{
  scanner_assert (s);

  // Dequeue and copy
  u8 next;
  bool more = cbuffer_dequeue (&next, s->chars_input);
  ASSERT (more);

  err_t_wrap (scanner_push_char_dcur (s, next, e), e);

  return SUCCESS;
}

/**
 * Peeks forward one char - returns
 * true if success, false otherwise
 */
static inline bool
scanner_peek (u8 *dest, scanner *s)
{
  scanner_assert (s);
  return cbuffer_peek_dequeue (dest, s->chars_input);
}

/**
 * Skips over chars until it finds non white space
 */
static inline void
consume_whitespace (scanner *s)
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
scanner_process_token_expect (scanner *s, token t)
{
  scanner_assert (s);

  ASSERT (cbuffer_avail (s->tokens_output) >= sizeof (token));
  u32 ret = cbuffer_write (&t, sizeof t, 1, s->tokens_output);
  ASSERT (ret == 1);

  s->state = SS_START;
}

////////////////////////////// State Machine Functions
static err_t
steady_state_execute (scanner *s, error *e)
{
  scanner_assert (s);

  // Pick up where you left off
  switch (s->state)
    {
    case SS_START:
      {
        return steady_state_start (s, e);
      }
    case SS_IDENT:
      {
        return steady_state_ident (s, e);
      }
    case SS_NUMBER:
      {
        return steady_state_number (s, e);
      }
    case SS_DECIMAL:
      {
        return steady_state_dec (s, e);
      }
    case SS_STRING:
      {
        return steady_state_string (s, e);
      }
    default:
      {
        UNREACHABLE ();
      }
    }
}

static err_t
process_string_or_ident (scanner *s, token_t type, error *e)
{
  scanner_assert (s);
  ASSERT (type == TT_STRING || type == TT_IDENTIFIER);

  string literal = (string){
    .data = s->str,
    .len = s->slen,
  };
  s->slen = 0; // len is captured in literal, safe to reset

  /**
   * Can only scan a string or ident if we're inside
   * a query
   */
  if (s->cur == NULL)
    {
      return error_causef (
          e, ERR_NOMEM,
          "%s: Encountered token: %s before an op code, no query space to allocate data onto.",
          TAG, tt_tostr (type));
    }

  /**
   * Allocate space to copy the token data
   */
  literal.data = lmalloc (&s->cur->alloc, literal.len, 1);
  if (literal.data == NULL)
    {
      return error_causef (
          e, ERR_NOMEM,
          "%s: Failed to allocate identifier token: %.*s into string space",
          TAG, s->slen, s->str);
    }

  // Copy data over
  i_memcpy (literal.data, s->str, literal.len);

  // Process the token
  token t = (token){
    .type = type,
    .str = literal,
  };
  scanner_process_token_expect (s, t);

  return SUCCESS;
}

static err_t
process_maybe_ident (scanner *s, error *e)
{
  scanner_assert (s);

  ASSERT (s->slen > 0);

  string literal = (string){
    .data = s->str,
    .len = s->slen,
  };

  // Check for magic tokens
  for (u32 i = 0; i < arrlen (magic_tokens); ++i)
    {
      if (string_equal (literal, magic_tokens[i].token))
        {
          // reset (literal is useless)
          s->slen = 0;
          scanner_process_token_expect (s, magic_tokens[i].t);
          return SUCCESS;
        }
    }

  // Check for op codes
  for (u32 i = 0; i < arrlen (op_codes); ++i)
    {
      if (string_equal (literal, op_codes[i].token))
        {
          token t = op_codes[i].t;

          // reset (literal is useless)
          s->slen = 0;

          // Allocate query space for downstream
          query *q;
          err_t_wrap (qspce_prvdr_get (s->qspcp, &q, tt_to_qt (t.type), e), e);
          s->cur = q; // Keep track of the current query to allocate later things to like strings and idents
          t.q = q;    // Assign the query to the token

          scanner_process_token_expect (s, t);
          return SUCCESS;
        }
    }

  // Otherwise it's an TT_IDENTIFIER
  return process_string_or_ident (s, TT_IDENTIFIER, e);
}

static err_t
steady_state_ident (scanner *s, error *e)
{
  scanner_assert (s);
  ASSERT (s->state == SS_IDENT);

  u8 c;

  while (scanner_peek (&c, s))
    {
      if (!is_alpha_num (c))
        {
          return process_maybe_ident (s, e);
        }

      err_t_wrap (scanner_cpy_advance_expect (s, e), e);
    }

  return SUCCESS; // no more chars to consume
}

static err_t
steady_state_string (scanner *s, error *e)
{
  scanner_assert (s);
  ASSERT (s->state == SS_STRING);

  u8 c;

  while (scanner_peek (&c, s))
    {
      if (c == '"')
        {
          scanner_advance_expect (s); // Skip over (last) quote
          return process_string_or_ident (s, TT_STRING, e);
        }

      err_t_wrap (scanner_cpy_advance_expect (s, e), e);
    }

  return SUCCESS;
}

static inline err_t
process_dec (scanner *s, error *e)
{
  scanner_assert (s);

  const string literal = (string){
    .data = s->str,
    .len = s->slen,
  };
  s->slen = 0;

  // Parse the float
  f32 dest;
  err_t_wrap (parse_f32_expect (&dest, literal, e), e);

  scanner_process_token_expect (s, tt_float (dest));

  return SUCCESS;
}

static err_t
steady_state_dec (scanner *s, error *e)
{
  scanner_assert (s);
  ASSERT (s->state == SS_DECIMAL);

  u8 c;
  while (scanner_peek (&c, s))
    {
      if (!is_num (c))
        {
          return process_dec (s, e);
        }
      err_t_wrap (scanner_cpy_advance_expect (s, e), e);
    }

  return SUCCESS;
}

static inline err_t
process_int (scanner *s, error *e)
{
  scanner_assert (s);

  const string literal = (string){
    .data = s->str,
    .len = s->slen,
  };
  s->slen = 0;

  // Parse the int
  i32 dest;
  err_t_wrap (parse_i32_expect (&dest, literal, e), e);

  scanner_process_token_expect (s, tt_integer (dest));

  return SUCCESS;
}

static err_t
steady_state_number (scanner *s, error *e)
{
  scanner_assert (s);
  ASSERT (s->state == SS_NUMBER);

  u8 c;

  while (scanner_peek (&c, s))
    {
      if (!is_num (c))
        {
          if (c == '.')
            {
              s->state = SS_DECIMAL;
              err_t_wrap (scanner_cpy_advance_expect (s, e), e);
              return steady_state_dec (s, e);
            }
          else
            {
              return process_int (s, e);
            }
        }
      err_t_wrap (scanner_cpy_advance_expect (s, e), e);
    }

  return SUCCESS;
}

static err_t
steady_state_start (scanner *s, error *e)
{
  scanner_assert (s);
  ASSERT (s->state == SS_START);

  consume_whitespace (s);

  u8 next;
  if (!scanner_peek (&next, s))
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
        scanner_advance_expect (s);
        scanner_process_token_expect (s, quick_tok (TT_SEMICOLON));
        break;
      }
    case '[':
      {
        scanner_advance_expect (s);
        scanner_process_token_expect (s, quick_tok (TT_LEFT_BRACKET));
        break;
      }
    case ']':
      {
        scanner_advance_expect (s);
        scanner_process_token_expect (s, quick_tok (TT_RIGHT_BRACKET));
        break;
      }
    case '{':
      {
        scanner_advance_expect (s);
        scanner_process_token_expect (s, quick_tok (TT_LEFT_BRACE));
        break;
      }
    case '}':
      {
        scanner_advance_expect (s);
        scanner_process_token_expect (s, quick_tok (TT_RIGHT_BRACE));
        break;
      }
    case '(':
      {
        scanner_advance_expect (s);
        scanner_process_token_expect (s, quick_tok (TT_LEFT_PAREN));
        break;
      }
    case ')':
      {
        scanner_advance_expect (s);
        scanner_process_token_expect (s, quick_tok (TT_RIGHT_PAREN));
        break;
      }
    case ',':
      {
        scanner_advance_expect (s);
        scanner_process_token_expect (s, quick_tok (TT_COMMA));
        break;
      }
    case '"':
      {
        s->slen = 0;
        scanner_advance_expect (s); // Skip the '"'
        s->state = SS_STRING;
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
            s->slen = 0;
            err_t_wrap (scanner_cpy_advance_expect (s, e), e);
            s->state = SS_IDENT;
            break;
          }
        else if (is_num (next) || next == '+' || next == '-')
          {
            /**
             * Advance forward once because steady state doesn't
             * account for + or -
             */
            s->slen = 0;
            err_t_wrap (scanner_cpy_advance_expect (s, e), e);
            s->state = SS_NUMBER;
            break;
          }
        else
          {
            return error_causef (
                e, ERR_SYNTAX,
                "%s: Unexpected char: %c",
                TAG, next);
          }
      }
    }
  return SUCCESS;
}

////////////////////////////// Main Functions
scanner
scanner_create (
    cbuffer *chars_input,
    cbuffer *tokens_output,
    qspce_prvdr *qspcp)
{
  scanner ret = {
    .state = SS_START,

    .chars_input = chars_input,
    .tokens_output = tokens_output,

    .slen = 0,

    .qspcp = qspcp,
    .cur = NULL,
  };
  scanner_assert (&ret);
  return ret;
}

err_t
scanner_execute (scanner *s, error *e)
{
  scanner_assert (s);

  while (true)
    {
      /**
       * Block on downstream
       */
      if (cbuffer_avail (s->tokens_output) < sizeof (token))
        {
          return SUCCESS;
        }

      /**
       * Block on upstream
       */
      if (cbuffer_len (s->chars_input) == 0)
        {
          return SUCCESS;
        }

      /**
       * Write a maximum of 1 token
       * or else handle error
       */
      err_t_wrap (steady_state_execute (s, e), e);
    }
}
