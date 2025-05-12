#include "compiler/scanner.h"
#include "compiler/tokens.h"
#include "dev/assert.h"
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

/**
 * Pushes a char to the end of the internal scanner string
 */
static inline err_t
scanner_push_char_dcur (scanner *s, char next, error *e)
{
  scanner_assert (s);
  i_log_trace ("Pushing char: %c\n", next);

  // Check room
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

/**
 * Initializes internal string
 */
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
  s->dcurcap = 10;
  s->dcurlen = 0;

  return SUCCESS;
}

/**
 * Resets, but does not free the string (let the downstream free it)
 */
static inline void
scanner_buffer_reset (scanner *s)
{
  scanner_assert (s);
  i_log_trace ("Resetting scanner internal string: %s\n", s->dcur);

  // Called on a non empty string
  ASSERT (s->dcur != NULL);
  ASSERT (s->dcurcap > 0);

  // INTENTIONALLY DONT FREE
  s->dcur = NULL;
  s->dcurlen = 0;
  s->dcurcap = 0;
}

scanner
scanner_create (scanner_params params)
{
  ASSERT (params.tokens_output->cap % sizeof (token) == 0);

  scanner ret = {

    .chars_input = params.chars_input,
    .tokens_output = params.tokens_output,

    .state = SS_START,

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

/**
 * Writes token to the output stream
 */
static inline void
scanner_write_token_expect (scanner *s, token t)
{
  scanner_assert (s);
  ASSERT (cbuffer_avail (s->tokens_output) >= sizeof (token));

  // Write the token
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

static err_t
ss_transition (scanner *s, scanner_state state, error *e)
{
  scanner_assert (s);

  // Update state
  s->state = state;

  // Block on output
  if (cbuffer_avail (s->tokens_output) < sizeof (token))
    {
      return SUCCESS;
    }

  switch (state)
    {
    case SS_START:
      {
        err_t_wrap (ss_start (s, e), e);
        break;
      }
    case SS_IDENT:
      {
        // Initialize internal string
        err_t_wrap (scanner_alloc_init (s, e), e);

        /**
         * We copy forward once so that we consume the first
         * // char which can only be alpha - then internally,
         * it's alpha numeric
         */
        err_t_wrap (scanner_cpy_advance_expect (s, e), e);
        err_t_wrap (ss_ident (s, e), e);
        break;
      }
    case SS_STRING:
      {
        err_t_wrap (scanner_alloc_init (s, e), e);
        scanner_advance_expect (s); // Skip over the first "
        err_t_wrap (ss_string (s, e), e);
        break;
      }
    case SS_NUMBER:
      {
        err_t_wrap (scanner_alloc_init (s, e), e);
        err_t_wrap (ss_number (s, e), e);
        break;
      }
    case SS_DECIMAL:
      {
        // No allocation - we were previously in SS_NUMBER
        err_t_wrap (ss_decimal (s, e), e);
        break;
      }
    default:
      {
        UNREACHABLE ();
      }
    }
  return SUCCESS;
}

// starts on the SECOND char of the sequence
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

  return SUCCESS;

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
          scanner_advance_expect (s); // Skip over quote
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

  /**
   * WARNING - This looks like it's causing
   * a dangling pointer but really I'm transferring
   * responsibility to the next block
   */
  scanner_buffer_reset (s);
  s->state = SS_START;

  scanner_write_token_expect (s, tt_string (literal));
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
              err_t_wrap (scanner_cpy_advance_expect (s, e), e);
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

  // Consume all whitespace
  consume_whitespace (s);

  // First check if there's data available
  u8 next;
  if (!scanner_peek (&next, s))
    {
      return SUCCESS;
    }

  i_log_trace ("Current state: %s. Next char: %c\n",
               scanner_state_to_str (s->state), next);

#define single_tok_continue(ttype)         \
  scanner_write_token_t_expect (s, ttype); \
  scanner_advance_expect (s);              \
  err_t_wrap (ss_transition (s, SS_START, e), e);

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
scanner_execute (scanner *s, error *e)
{
  scanner_assert (s);

  // Pick up where you left off
  switch (s->state)
    {
    case SS_START:
      {
        err_t_wrap (ss_start (s, e), e);
        break;
      }
    case SS_IDENT:
      {
        err_t_wrap (ss_ident (s, e), e);
        break;
      }
    case SS_NUMBER:
      {
        err_t_wrap (ss_number (s, e), e);
        break;
      }
    case SS_DECIMAL:
      {
        err_t_wrap (ss_decimal (s, e), e);
        break;
      }
    case SS_STRING:
      {
        err_t_wrap (ss_string (s, e), e);
        break;
      }
    }
  return SUCCESS;
}

void
scanner_release (scanner *s)
{
  (void)s;
  panic ();
}
