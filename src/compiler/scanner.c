#include "compiler/scanner.h"
#include "compiler/tokens.h"
#include "dev/assert.h"
#include "ds/cbuffer.h"
#include "errors/error.h"
#include "intf/logging.h"
#include "mm/lalloc.h"
#include "utils/macros.h"
#include "utils/numbers.h"

////////////////////// SCANNER (chars -> tokens)

DEFINE_DBG_ASSERT_I (scanner, scanner, s)
{
  ASSERT (s);

  if (s->dcur)
    {
      ASSERT (s->dcur);
      ASSERT (s->dcurlen <= s->dcurcap);
      ASSERT (s->dcurcap > 0);
    }
  else
    {
      ASSERT (s->dcur == NULL);
      ASSERT (s->dcurlen == 0);
      ASSERT (s->dcurcap == 0);
    }
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

  if (s->dcurlen == s->dcurcap)
    {
      lalloc_r next = lrealloc (
          s->string_allocator,
          s->dcur,
          2 * s->dcurcap,
          s->dcurcap + 1,
          sizeof *s->dcur);

      if (next.stat != AR_SUCCESS)
        {
          return error_causef (
              e,
              ERR_NOMEM,
              "Scanner: "
              "Failed to realloc internal dynamic string");
        }

      s->dcur = next.ret;
      s->dcurcap = next.rlen;
    }

  // Append string
  s->dcur[s->dcurlen++] = next;

  return SUCCESS;
}

static inline err_t
scanner_alloc_init (scanner *s, error *e)
{
  scanner_assert (s);

  // Called on an empty string
  ASSERT (s->dcur == NULL);

  lalloc_r data = lmalloc (
      s->string_allocator,
      10, 10,
      sizeof *s->dcur);

  if (data.stat != AR_SUCCESS)
    {
      return error_causef (
          e, ERR_NOMEM,
          "Scanner: "
          "Failed to allocate dynamic string");
    }

  s->dcur = data.ret;
  s->dcurcap = data.rlen;
  s->dcurlen = 0;

  return SUCCESS;
}

static inline void
scanner_buffer_reset (scanner *s)
{
  scanner_assert (s);

  // Called on a non empty string
  ASSERT (s->dcur != NULL);
  ASSERT (s->dcurcap > 0);

  /**
   * WARNING - This looks like it's causing
   * a dangling pointer but really I'm transferring
   * responsibility to down the chain
   */
  s->dcur = NULL;
  s->dcurlen = 0;
  s->dcurcap = 0;
}

scanner
scanner_create (scanner_params params)
{
  ASSERT (params.tokens_output->cap % sizeof (token) == 0);

  scanner ret = {
    .e = error_create (NULL),
    .state = SS_START,

    .chars_input = params.chars_input,
    .tokens_output = params.tokens_output,

    .dcur = NULL,
    .dcurlen = 0,
    .dcurcap = 0,

    .string_allocator = params.string_allocator,
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
  ASSERT (s->dcur);

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

static inline void
scanner_write_token_t_expect (scanner *s, token_t t)
{
  scanner_assert (s);
  ASSERT (cbuffer_avail (s->tokens_output) >= sizeof (token));
  scanner_write_token_expect (s, quick_tok (t));
}

typedef struct
{
  char *data;
  u32 len;
  prim_t type;
} prim_token;

// Maybe a trie for faster checks?
// TODO - move this to prim_t module
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

  { .data = "cf32", .len = 4, .type = CF32 },
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
};

typedef struct
{
  char *data;
  u32 len;
  token_t type;
} magic_token;

// Maybe a trie
static const magic_token magic_tokens[] = {
  // Json commands
  { .data = "create", .len = 6, .type = TT_CREATE },
  { .data = "delete", .len = 6, .type = TT_DELETE },
  { .data = "append", .len = 6, .type = TT_APPEND },
  { .data = "insert", .len = 6, .type = TT_INSERT },
  { .data = "update", .len = 6, .type = TT_UPDATE },
  { .data = "read", .len = 4, .type = TT_READ },
  { .data = "take", .len = 4, .type = TT_TAKE },

  // Binary commands
  { .data = "bcreate", .len = 7, .type = TT_BCREATE },
  { .data = "bdelete", .len = 7, .type = TT_BDELETE },
  { .data = "bappend", .len = 7, .type = TT_BAPPEND },
  { .data = "binsert", .len = 7, .type = TT_BINSERT },
  { .data = "bupdate", .len = 7, .type = TT_BUPDATE },
  { .data = "bread", .len = 5, .type = TT_BREAD },
  { .data = "btake", .len = 5, .type = TT_BTAKE },

  // Types
  { .data = "struct", .len = 6, .type = TT_STRUCT },
  { .data = "union", .len = 5, .type = TT_UNION },
  { .data = "enum", .len = 4, .type = TT_ENUM },
};

static err_t ss_transition (scanner *s, scanner_state state, error *e);
static err_t ss_start (scanner *s, error *e);
static err_t ss_ident (scanner *s, error *e);
static err_t ss_string (scanner *s, error *e);
static err_t ss_number (scanner *s, error *e);
static err_t ss_decimal (scanner *s, error *e);

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
        err_t_wrap (scanner_alloc_init (s, e), e);

        // Write the first char to the array
        err_t_wrap (scanner_cpy_advance_expect (s, e), e);
        break;
      }
    case SS_STRING:
      {
        // Initialize output string
        err_t_wrap (scanner_alloc_init (s, e), e);

        // Skip over the first quotation mark
        scanner_advance_expect (s);
        break;
      }
    case SS_NUMBER:
      {
        // Initialize output string (to parse later)
        err_t_wrap (scanner_alloc_init (s, e), e);
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
ss_ident (scanner *s, error *e)
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

  ASSERT (s->dcurlen > 0);
  ASSERT (s->dcur);

  string literal = (string){
    .data = s->dcur,
    .len = s->dcurlen,
  };

  scanner_buffer_reset (s);

  // Check for magic tokens
  for (u32 i = 0; i < arrlen (magic_tokens); ++i)
    {
      if (string_equal (
              literal,
              (string){
                  .data = magic_tokens[i].data,
                  .len = magic_tokens[i].len,
              }))
        {
          scanner_write_token_t_expect (s, magic_tokens[i].type);
          lfree (s->string_allocator, literal.data);
          goto theend;
        }
    }

  // Check for primitives
  for (u32 i = 0; i < arrlen (prim_tokens); ++i)
    {
      if (string_equal (
              literal,
              (string){
                  .data = prim_tokens[i].data,
                  .len = prim_tokens[i].len,
              }))
        {
          scanner_write_token_expect (s, tt_prim (prim_tokens[i].type));
          lfree (s->string_allocator, literal.data);
          goto theend;
        }
    }

  // Otherwise, write an ident token
  scanner_write_token_expect (s, tt_ident (literal));

theend:
  err_t_wrap (ss_transition (s, SS_START, e), e);
  return SUCCESS;
}

static err_t
ss_string (scanner *s, error *e)
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

  ASSERT (s->dcur);

  string literal = (string){
    .data = s->dcur,
    .len = s->dcurlen,
  };

  scanner_buffer_reset (s);
  scanner_write_token_expect (s, tt_string (literal));

  err_t_wrap (ss_transition (s, SS_START, e), e);

  return SUCCESS;
}

static err_t
ss_decimal (scanner *s, error *e)
{
  scanner_assert (s);
  ASSERT (s->state == SS_DECIMAL);

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
          f32 dest = parse_f32_expect (literal);

          // Reset and write
          lfree (s->string_allocator, s->dcur);
          scanner_buffer_reset (s);
          scanner_write_token_expect (s, tt_float (dest));

          // TODO - should I require whitespace after numbers?
          err_t_wrap (ss_transition (s, SS_START, e), e);
          return SUCCESS;
        }
      err_t_wrap (scanner_cpy_advance_expect (s, e), e);
    }

  return SUCCESS;
}

static err_t
ss_number (scanner *s, error *e)
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
              return SUCCESS;
            }
          else
            {
              const string literal = (string){
                .data = s->dcur,
                .len = s->dcurlen,
              };

              // Parse the int
              i32 dest = parse_i32_expect (literal);

              // Reset and write
              lfree (s->string_allocator, s->dcur);
              scanner_buffer_reset (s);
              scanner_write_token_expect (s, tt_integer (dest));

              err_t_wrap (ss_transition (s, SS_START, e), e);
              return SUCCESS;
            }
        }
      err_t_wrap (scanner_cpy_advance_expect (s, e), e);
    }

  return SUCCESS;
}

static err_t
ss_start (scanner *s, error *e)
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

#define single_tok_continue(ttype)                    \
  do                                                  \
    {                                                 \
      scanner_advance_expect (s);                     \
      scanner_write_token_t_expect (s, ttype);        \
      err_t_wrap (ss_transition (s, SS_START, e), e); \
    }                                                 \
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
        return ss_start (s, e);
      }
    case SS_IDENT:
      {
        return ss_ident (s, e);
      }
    case SS_NUMBER:
      {
        return ss_number (s, e);
      }
    case SS_DECIMAL:
      {
        return ss_decimal (s, e);
      }
    case SS_STRING:
      {
        return ss_string (s, e);
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
      if (scanner_execute_state (s, &s->e))
        {
          error_log_consume (&s->e);
          scanner_write_token_expect (s, quick_tok (TT_ERROR));
          err_t ret = ss_transition (s, SS_START, &s->e);
          ASSERT (ret == SUCCESS);
          return;
        }
    }
}
