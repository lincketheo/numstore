#include "compiler/scanner.h"

#include "compiler/tokens.h" // token

#include "dev/assert.h"    // DEFINE_DBG_ASSERT_I
#include "ds/cbuffer.h"    // cbuffer
#include "errors/error.h"  // err_t
#include "intf/stdlib.h"   // i_memcpy
#include "utils/macros.h"  // is_alpha
#include "utils/numbers.h" // parse_i32_expect

////////////////////// SCANNER (chars -> tokens)

DEFINE_DBG_ASSERT_I (scanner, scanner, s)
{
  ASSERT (s);
  ASSERT (s->slen <= 512);
}

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

static inline err_t
scanner_push_char_dcur (scanner *s, char next, error *e)
{
  scanner_assert (s);
  i_log_trace ("Pushing char: %c\n", next);

  if (s->slen == 512)
    {
      return error_causef (
          e, ERR_NOMEM,
          "Scanner: "
          "ident buffer overflow");
    }

  s->str[s->slen++] = next;

  return SUCCESS;
}

scanner
scanner_create (
    cbuffer *chars_input,
    cbuffer *tokens_output,
    stmtctrl *ctrl)
{
  scanner ret = {
    .state = SS_START,

    .chars_input = chars_input,
    .tokens_output = tokens_output,

    .slen = 0,

    .ctrl = ctrl,
  };
  scanner_assert (&ret);
  return ret;
}

/////////////////////////////////// UTILS

/**
 * Advances forward one char
 * ASSERTS that it can actually do that
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
scanner_write_token_expect (scanner *s, token t)
{
  scanner_assert (s);
  ASSERT (cbuffer_avail (s->tokens_output) >= sizeof (token));

  u32 ret = cbuffer_write (&t, sizeof t, 1, s->tokens_output);
  ASSERT (ret == 1);
}

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

// helper to infer string length at compile time
#define STR_LIT(s)        \
  {                       \
    (sizeof (s) - 1), (s) \
  }

static const prim_token prim_tokens[] = {
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

static const magic_token magic_tokens[] = {
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

  // types
  { .token = STR_LIT ("struct"), .t = { .type = TT_STRUCT } },
  { .token = STR_LIT ("union"), .t = { .type = TT_UNION } },
  { .token = STR_LIT ("enum"), .t = { .type = TT_ENUM } },
};

static err_t ss_transition (scanner *s, scanner_state state, error *e);
static err_t steady_state_start (scanner *s, error *e);
static err_t steady_state_ident (scanner *s, error *e);
static err_t steady_state_string (scanner *s, error *e);
static err_t steady_state_number (scanner *s, error *e);
static err_t steady_state_dec (scanner *s, error *e);

/**
 * We expect to have at least 1 char available here
 */
static err_t
ss_transition (scanner *s, scanner_state state, error *e)
{
  scanner_assert (s);

  switch (state)
    {
    case SS_START:
      {
        break;
      }
    case SS_IDENT:
      {
        // initialize output string
        s->slen = 0;

        // Write the first char to the array
        err_t_wrap (scanner_cpy_advance_expect (s, e), e);
        break;
      }
    case SS_STRING:
      {
        // Initialize output string
        s->slen = 0;

        // Skip over the first quotation mark
        scanner_advance_expect (s);
        break;
      }
    case SS_NUMBER:
      {
        // Initialize output string (to parse later)
        s->slen = 0;
        break;
      }
    case SS_DECIMAL:
      {
        // No allocation - we were previously in SS_NUMBER
        // Skip over the "."
        err_t_wrap (scanner_cpy_advance_expect (s, e), e);
        break;
      }
    default:
      {
        UNREACHABLE ();
      }
    }

  s->state = state;
  return SUCCESS;
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
          goto finish;
        }

      err_t_wrap (scanner_cpy_advance_expect (s, e), e);
    }

  return SUCCESS; // no more chars to consume

finish:
  scanner_assert (s);

  ASSERT (s->slen > 0);

  string literal = (string){
    .data = s->str,
    .len = s->slen,
  };
  s->slen = 0;

  // Check for magic tokens
  for (u32 i = 0; i < arrlen (magic_tokens); ++i)
    {
      if (string_equal (literal, magic_tokens[i].token))
        {
          scanner_write_token_expect (s, magic_tokens[i].t);
          goto theend;
        }
    }

  // Check for primitives
  for (u32 i = 0; i < arrlen (prim_tokens); ++i)
    {
      if (string_equal (literal, prim_tokens[i].token))
        {
          scanner_write_token_expect (s, prim_tokens[i].t);
          goto theend;
        }
    }

  // Otherwise, write an ident token
  literal.data = lmalloc (&s->ctrl->strs_alloc, literal.len, 1);
  if (literal.data == NULL)
    {
      return error_causef (
          e, ERR_NOMEM,
          "Scanner: "
          "Failed to allocate identifier token: %.*s into string space",
          s->slen, s->str);
    }
  i_memcpy (literal.data, s->str, literal.len);

  scanner_write_token_expect (s, tt_ident (literal));

theend:
  s->slen = 0;
  err_t_wrap (ss_transition (s, SS_START, e), e);
  return SUCCESS;
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
          goto finish;
        }

      err_t_wrap (scanner_cpy_advance_expect (s, e), e);
    }

  return SUCCESS;

finish:
  scanner_assert (s);

  ASSERT (s->slen > 0);

  string literal = {
    .data = lmalloc (&s->ctrl->strs_alloc, s->slen, 1),
    .len = s->slen,
  };
  s->slen = 0;

  if (literal.data == NULL)
    {
      return error_causef (
          e, ERR_NOMEM,
          "Scanner: "
          "Failed to allocate string token: %.*s into query space",
          s->slen, s->str);
    }
  i_memcpy (literal.data, s->str, literal.len);

  scanner_write_token_expect (s, tt_string (literal));

  err_t_wrap (ss_transition (s, SS_START, e), e);

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
          goto finished;
        }
      err_t_wrap (scanner_cpy_advance_expect (s, e), e);
    }

  return SUCCESS; // Nothing else, but in the middle

finished:
  scanner_assert (s);

  const string literal = (string){
    .data = s->str,
    .len = s->slen,
  };
  s->slen = 0;

  // Parse the float
  f32 dest;
  err_t_wrap (parse_f32_expect (&dest, literal, e), e);

  scanner_write_token_expect (s, tt_float (dest));

  // TODO - should I require whitespace after numbers?
  err_t_wrap (ss_transition (s, SS_START, e), e);

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
              err_t_wrap (ss_transition (s, SS_DECIMAL, e), e);
              return steady_state_dec (s, e);
            }
          else
            {
              goto finished;
            }
        }
      err_t_wrap (scanner_cpy_advance_expect (s, e), e);
    }

  return SUCCESS;

finished:
  scanner_assert (s);

  const string literal = (string){
    .data = s->str,
    .len = s->slen,
  };
  s->slen = 0;

  // Parse the int
  i32 dest;
  err_t_wrap (parse_i32_expect (&dest, literal, e), e);

  scanner_write_token_expect (s, tt_integer (dest));

  err_t_wrap (ss_transition (s, SS_START, e), e);

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
      // no more tokens
      return SUCCESS;
    }

#define single_tok_continue(ttype)                       \
  do                                                     \
    {                                                    \
      scanner_advance_expect (s);                        \
      scanner_write_token_expect (s, quick_tok (ttype)); \
      err_t_wrap (ss_transition (s, SS_START, e), e);    \
    }                                                    \
  while (0)

  switch (next)
    {
    case ';':
      {
        single_tok_continue (TT_SEMICOLON);
        break;
      }
    case '[':
      {
        single_tok_continue (TT_LEFT_BRACKET);
        break;
      }
    case ']':
      {
        single_tok_continue (TT_RIGHT_BRACKET);
        break;
      }
    case '{':
      {
        single_tok_continue (TT_LEFT_BRACE);
        break;
      }
    case '}':
      {
        single_tok_continue (TT_RIGHT_BRACE);
        break;
      }
    case '(':
      {
        single_tok_continue (TT_LEFT_PAREN);
        break;
      }
    case ')':
      {
        single_tok_continue (TT_RIGHT_PAREN);
        break;
      }
    case ',':
      {
        single_tok_continue (TT_COMMA);
        break;
      }
    case '"':
      {
        err_t_wrap (ss_transition (s, SS_STRING, e), e);
        break;
      }
    default:
      {
        if (is_alpha (next))
          {
            // Due to maximal munch all magic strings
            // are treated as ident - then checked at the end
            err_t_wrap (ss_transition (s, SS_IDENT, e), e);
            break;
          }
        else if (is_num (next) || next == '+' || next == '-')
          {
            err_t_wrap (ss_transition (s, SS_NUMBER, e), e);
            break;
          }
        else
          {
            return error_causef (
                e, ERR_SYNTAX,
                "Scanner: "
                "Unexpected char: %c",
                next);
          }
      }
    }
#undef single_tok_continue
  return SUCCESS;
}

err_t
scanner_execute_state (scanner *s, error *e)
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

void
scanner_execute (scanner *s)
{
  scanner_assert (s);

  switch (s->ctrl->state)
    {
    case STCTRL_ERROR:
      {
        // Discard elements
        cbuffer_discard_all (s->chars_input);
        return;
      }
    case STCTRL_WRITING:
      {
        // Expect to be done
        ASSERT (cbuffer_len (s->chars_input) == 0);
        return;
      }
    case STCTRL_EXECTUING:
      {
        break;
      }
    }

  while (true)
    {
      /**
       * Block on downstream
       */
      if (cbuffer_avail (s->tokens_output) < sizeof (token))
        {
          return;
        }

      /**
       * Block on upstream (all handlers are greedy)
       */
      if (cbuffer_len (s->chars_input) == 0)
        {
          return;
        }

      /**
       * Write a maximum of 1 token
       * or else handle error
       */
      if (scanner_execute_state (s, &s->ctrl->e))
        {
          err_t ret = ss_transition (s, SS_START, NULL);
          ASSERT (ret == SUCCESS);

          s->ctrl->state = STCTRL_ERROR;
          return;
        }
    }
}
