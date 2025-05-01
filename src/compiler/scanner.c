#include "compiler/scanner.h"
#include "compiler/tokens.h"
#include "dev/assert.h"
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

static inline bool
scanner_alloc_init (scanner *s)
{
  scanner_assert (s);
  ASSERT (s->dcur == NULL);

  char *data = lmalloc (s->string_allocator, 10 * sizeof *data);
  if (!data)
    {
      return false;
    }

  s->dcur = data;
  s->dcurcap = 10;
  s->dcurlen = 0;

  return true;
}

static inline void
scanner_buffer_reset (scanner *s)
{
  scanner_assert (s);
  ASSERT (s->dcur != NULL);
  ASSERT (s->dcurcap > 0);

  // INTENTIONALLY DONT FREE
  s->dcur = NULL;
  s->dcurlen = 0;
  s->dcurcap = 0;
}

#define MAX_STRING_SIZE 1000

void
scanner_create (scanner *dest, scanner_params params)
{
  ASSERT (dest);
  ASSERT (params.tokens_output->cap % sizeof (token) == 0);

  dest->chars_input = params.chars_input;
  dest->tokens_output = params.tokens_output;

  dest->state = SS_START;

  dest->dcur = NULL;
  dest->dcurlen = 0;
  dest->dcurcap = 0;

  dest->string_allocator = params.string_allocator;
  scanner_assert (dest);
}

// Advances forward one char
static inline void
scanner_advance_expect (scanner *s)
{
  scanner_assert (s);
  u8 next;
  ASCOPE (bool more =)
  cbuffer_dequeue (&next, s->chars_input);
  ASSERT (more);
}

// Advances forward one char and copies it to internal string
static inline bool
scanner_cpy_advance_expect (scanner *s)
{
  scanner_assert (s);
  ASSERT (s->dcur);

  // Dequeue and copy
  u8 next;
  ASCOPE (int more =)
  cbuffer_dequeue (&next, s->chars_input);
  ASSERT (more);

  // Check room
  if (s->dcurlen == s->dcurcap)
    {
      char *next = lrealloc (s->string_allocator, s->dcur, 2 * s->dcurcap * sizeof *next);
      if (!next)
        {
          return false; // No memory - block
        }
      s->dcur = next;
      s->dcurcap = 2 * s->dcurcap;
    }

  s->dcur[s->dcurlen++] = next;

  return true;
}

// Advance forward one char if there is a char and can alloc data
// returns true if advanced and copied false else
static inline bool
scanner_cpy_advance (scanner *s)
{
  scanner_assert (s);
  ASSERT (s->dcur);

  // Dequeue and copy
  u8 next;
  if (!cbuffer_dequeue (&next, s->chars_input))
    {
      return false;
    }

  // Check room
  if (s->dcurlen == s->dcurcap)
    {
      char *dcur = lrealloc (s->string_allocator, s->dcur, 2 * s->dcurcap * sizeof *dcur);
      if (!dcur)
        {
          return false; // No memory - block
        }
      s->dcur = dcur;
      s->dcurcap = 2 * s->dcurcap;
    }

  s->dcur[s->dcurlen++] = next;

  return true;
}

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

  ASCOPE (u32 ret =)
  cbuffer_write (&t, sizeof t, 1, s->tokens_output);
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

  { .data = "bool", .len = 4, .type = BOOL },
  { .data = "bit", .len = 3, .type = BIT },
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

static void ss_transition (scanner *s, scanner_state state);
static void ss_start (scanner *s);
static void ss_ident (scanner *s);
static void ss_string (scanner *s);
static void ss_number (scanner *s);
static void ss_decimal (scanner *s);
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
    case SS_IDENT:
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
        ss_ident (s);
        break;
      }
    case SS_STRING:
      {
        if (!scanner_alloc_init (s))
          {
            return;
          }
        scanner_advance_expect (s); // Skip over "
        ss_string (s);
        break;
      }
    case SS_NUMBER:
      {
        if (!scanner_alloc_init (s))
          {
            return;
          }
        // Advance once first
        if (!scanner_cpy_advance (s))
          {
            return;
          }
        ss_number (s);
        break;
      }
    case SS_DECIMAL:
      {
        ss_decimal (s);
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
ss_ident (scanner *s)
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
          scanner_write_token_t_expect (s, magic_tokens[i].type);
          lfree (s->string_allocator, literal.data);
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
          scanner_write_token_expect (s, tt_prim (prim_tokens[i].type));
          lfree (s->string_allocator, literal.data);
          goto theend;
        }
    }

  scanner_write_token_expect (s, tt_ident (literal));

theend:
  ss_transition (s, SS_START);
  return;
}

static void
ss_string (scanner *s)
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

      if (!scanner_cpy_advance_expect (s))
        {
          return;
        }
    }

  return;

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
}

static void
ss_decimal (scanner *s)
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
          f32 dest;
          err_t ret = parse_f32_expect (&dest, literal);
          if (ret)
            {
              ss_transition (s, SS_ERROR_REWIND);
              return;
            }

          // Reset and write
          lfree (s->string_allocator, s->dcur);
          scanner_buffer_reset (s);
          scanner_write_token_expect (s, tt_float (dest));

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
ss_number (scanner *s)
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
              if (!scanner_cpy_advance_expect (s))
                {
                  return;
                }

              ss_transition (s, SS_DECIMAL);
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
              lfree (s->string_allocator, s->dcur);
              scanner_buffer_reset (s);
              scanner_write_token_expect (s, tt_integer (dest));

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
  consume_whitespace (s);

  // First check if there's data available
  u8 next;
  if (!scanner_peek (&next, s))
    {
      return;
    }

#define single_tok_continue(ttype)         \
  scanner_write_token_t_expect (s, ttype); \
  scanner_advance_expect (s);              \
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
    case '"':
      {

        ss_transition (s, SS_STRING);
        return;
      }
    default:
      {
        if (is_alpha (next))
          {
            ss_transition (s, SS_IDENT);
            return;
          }
        else if (is_num (next) || next == '+' || next == '-')
          {

            ss_transition (s, SS_NUMBER);
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
        break;
      }
    case SS_IDENT:
      {
        ss_transition (s, SS_IDENT);
        break;
      }
    case SS_NUMBER:
      {
        ss_transition (s, SS_NUMBER);
        break;
      }
    case SS_DECIMAL:
      {
        ss_transition (s, SS_DECIMAL);
        break;
      }
    case SS_STRING:
      {
        ss_transition (s, SS_STRING);
        break;
      }
    case SS_ERROR_REWIND:
      {
        ss_transition (s, SS_ERROR_REWIND);
        break;
      }
    }
}

void
scanner_release (scanner *s)
{
  (void)s;
}
