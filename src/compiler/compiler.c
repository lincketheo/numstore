#include "compiler/compiler.h"

#include "dev/assert.h"    // DEFINE_DBG_ASSERT_I
#include "ds/cbuffer.h"    // cbuffer
#include "errors/error.h"  // err_t
#include "intf/stdlib.h"   // i_memcpy
#include "utils/macros.h"  // is_alpha
#include "utils/numbers.h" // parse_i32_expect

DEFINE_DBG_ASSERT_I (compiler, compiler, s)
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
 * call [compiler_process_token], which resets
 * to the SS_START state (and maybe writes the token)
 *
 * Then it's done. ss_transition returns and is not
 * recursive (to save the parser_stack), so you need to continue
 * calling ss_execute until your input buffer is empty
 */
// Picks which function below to execute
static err_t steady_state_execute (compiler *s, error *e);
static err_t steady_state_start (compiler *s, error *e);
static err_t steady_state_ident (compiler *s, error *e);
static err_t steady_state_string (compiler *s, error *e);
static err_t steady_state_number (compiler *s, error *e);
static err_t steady_state_dec (compiler *s, error *e);

//////////////////////////////////////////// PARSER UTILS

/**
 * Pops the top ast_parser off the stack
 * and builds it. Returns error if build fails
 *
 * Error:
 *  - SPR_SYNTAX_ERROR (ERR_SYNTAX)
 *  - SPR_MALLOC_ERROR (ERR_NOMEM)
 *
 * Does not return SPR_CONTINUE
 */
static inline ast_result
compiler_pop (compiler *sp)
{
  compiler_assert (sp);
  ASSERT (sp->sp > 0);

  // Pop the top off the stack
  ast_parser top = sp->parser_stack[--sp->sp];

  // Build the top
  ast_result ret = ast_parser_to_result (&top);

  // Reset the allocator (free all the scratch work)
  lalloc_reset_to_state (&sp->parser_work, top.alloc_start);

  // Return a result, not parser
  return ret;
}

/**
 * Gets just the reference to the top most parser
 */
static inline ast_parser *
compiler_top_ref (compiler *sp)
{
  compiler_assert (sp);
  ASSERT (sp->sp > 0);

  return &sp->parser_stack[sp->sp - 1];
}

static inline void
push_parser_expect (compiler *p, ast_parser value)
{
  compiler_assert (p);
  ASSERT (p->sp < 20);
  p->parser_stack[p->sp++] = value;
}

/**
 * Pushes a new parser to the top of the stack
 */
static err_t
push_parser (compiler *p, ast_parser value, error *e)
{
  compiler_assert (p);

  if (p->sp == 20)
    {
      return error_causef (
          e, ERR_NOMEM,
          "Parser: parser_stack overflow");
    }

  push_parser_expect (p, value);

  return SUCCESS;
}

/**
 * Pushes a type parser to the stack
 * Basically just a wrapper around push_parser
 * with type_parser constructor
 */
static inline err_t
push_type_parser (compiler *p, query cur, error *e)
{
  compiler_assert (p);

  ast_parser value = {
    .tb = (type_parser){
        .state = TB_UNKNOWN,
    },
    .type = SBBT_TYPE,
    .cur = cur,
    .alloc_start = lalloc_get_state (&p->parser_work),
  };

  return push_parser (p, value, e);
}

/**
 * Pushes a type parser to the stack
 * Basically just a wrapper around push_parser
 * with query_parser constructor
 */
static inline void
push_query_parser_start (compiler *sp)
{
  compiler_assert (sp);

  ast_parser top = {
    .qb = (query_parser){
        .state = QP_UNKNOWN,
    },
    .type = SBBT_QUERY,
    .cur = (query){ 0 },
    .alloc_start = lalloc_get_state (&sp->parser_work),
  };
  ASSERT (sp->sp == 0);

  push_parser_expect (sp, top);
}

//////////////////////////////////////////// Parser Main

/**
 * Pushes a new token onto the parser stack
 *
 * Returns result of that feed for the top most
 * element of the stack
 *
 * Note, the return value is SPR_DONE iff the
 * stack is size 1 and the bottom (top) element is done
 */
static stackp_result
parser_feed_token (compiler *tp, token t, error *e)
{
  compiler_assert (tp);
  ASSERT (tp->sp > 0);

  /**
   * First, get the top ast compiler from the
   * parser_stack - and ask: "Hey compiler, do you
   * want a token or a type next?"
   */
  ast_parser *top = compiler_top_ref (tp);
  sb_feed_t exp = ast_parser_expect_next (top, t);

  /**
   * Top Parser wants a type, we push a new
   * type_parser onto the top of the parser_stack
   */
  if (exp == SBFT_TYPE)
    {
      // Push a new type parser
      err_t ret = push_type_parser (tp, top->cur, e);
      if (ret < 0)
        {
          return (stackp_result)ret;
        }

      // Get the new top
      top = compiler_top_ref (tp);

      // All type parser accept a token next
      // This could change
      ASSERT (ast_parser_expect_next (top, t) == SBFT_TOKEN);
    }

  // Give the top a token
  stackp_result ret = ast_parser_accept_token (
      top, t, &tp->parser_work, e);

  if (ret < 0)
    {
      return ret;
    }

  /**
   * If we just finished the top compiler,
   * we loop and build each item on the
   * parser_stack then feed it to the compiler
   * beneath it
   *
   * sp == 1 is done because we've reached the base
   */
  while (tp->sp > 1 && ret == SPR_DONE)
    {
      // Pop the top off the parser_stack
      ast_result top = compiler_pop (tp);

      // compiler_pop only returns DONE or ERROR, not CONTINUE
      ASSERT (ret == SPR_DONE);

      /**
       * Merge whatever we just built
       * into the next element
       */
      switch (top.type)
        {
        case SBBT_TYPE:
          {
            ret = ast_parser_accept_type (
                &tp->parser_stack[tp->sp - 1], top.t, e);
            break;
          }
        case SBBT_QUERY:
          {
            // Don't support recursive queries yet
            panic ();
            break;
          }
        }

      if (ret < 0)
        {
          return ret;
        }
      continue;
    }

  return ret;
}

/**
 * The parser_stack is one element high
 * and the only element that lives on it is
 * done and valid
 */
static inline query
compiler_get (compiler *sp)
{
  compiler_assert (sp);
  ASSERT (sp->sp == 1);

  /**
   * Pop the last element off the parser_stack -
   * it should be a query
   */
  ast_result top = compiler_pop (sp);
  ASSERT (top.type == SBBT_QUERY);

  return top.q;
}

static inline void
compiler_write_out (compiler *p)
{
  compiler_result res = {
    .ok = true,
    .query = compiler_get (p),
  };

  push_query_parser_start (p);

  u32 written = cbuffer_write (&res, sizeof res, 1, &p->output);
  ASSERT (written == 1);
}

static err_t
compiler_feed_and_translate_token (compiler *p, token tok, error *e)
{
  switch (parser_feed_token (p, tok, e))
    {
    case SPR_DONE:
      {
        compiler_write_out (p);
        return SUCCESS;
      }
    case SPR_CONTINUE:
      {
        return SUCCESS; // Do nothing
      }
    case SPR_NOMEM:
    case SPR_SYNTAX_ERROR:
      {
        return err_t_from (e);
      }
    }
  UNREACHABLE ();
}

/////////////////////////////////// UTILS
/// Some simple tools to help scanning like special
/// advance functions etc.

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
    }
  UNREACHABLE ();
}

/**
 * Pushes [next] onto the internal string
 * that compiler holds
 *
 * Errors:
 *    - ERR_NOMEM - internal parser_stack overflows
 */
static inline err_t
compiler_push_char (compiler *s, char next, error *e)
{
  compiler_assert (s);

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
compiler_cpy_advance_expect (compiler *s, error *e)
{
  compiler_assert (s);

  // Dequeue and copy
  u8 next;
  bool more = cbuffer_dequeue (&next, &s->input);
  ASSERT (more);

  err_t_wrap (compiler_push_char (s, next, e), e);

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
compiler_process_token (compiler *s, token t, error *e)
{
  compiler_assert (s);

  err_t_wrap (compiler_feed_and_translate_token (s, t, e), e);

  ASSERT (cbuffer_avail (&s->output) >= sizeof (token));
  u32 ret = cbuffer_write (&t, sizeof t, 1, &s->output);
  ASSERT (ret == 1);

  s->state.state = SS_START;

  return SUCCESS;
}

////////////////////////////// State Machine Functions
static err_t
steady_state_execute (compiler *s, error *e)
{
  compiler_assert (s);

  // Pick up where you left off
  switch (s->state.state)
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
process_string_or_ident (compiler *s, token_t type, error *e)
{
  compiler_assert (s);
  ASSERT (type == TT_STRING || type == TT_IDENTIFIER);

  string literal = (string){
    .data = s->str,
    .len = s->slen,
  };
  s->slen = 0; // len is captured in literal, safe to reset

  // Process the token
  token t = (token){
    .type = type,
    .str = literal,
  };

  return compiler_process_token (s, t, e);
}

/**
 * Processes a string.
 * It could be:
 *  1. An Ident (any string like token that isn't well known/magic)
 *  2. Magic word (struct, union etc)
 *  3. String ("foobar")
 */
static err_t
process_maybe_ident (compiler *s, error *e)
{
  compiler_assert (s);

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
          return compiler_process_token (s, magic_tokens[i].t, e);
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
          query q;
          err_t_wrap (query_provider_get (
                          s->qp, &q,
                          tt_to_qt (t.type), e),
                      e);

          t.q = q;

          return compiler_process_token (s, t, e);
        }
    }

  // Otherwise it's an TT_IDENTIFIER
  return process_string_or_ident (s, TT_IDENTIFIER, e);
}

static err_t
steady_state_ident (compiler *s, error *e)
{
  compiler_assert (s);
  ASSERT (s->state.state == SS_IDENT);

  u8 c;

  while (compiler_peek (&c, s))
    {
      if (!is_alpha_num (c))
        {
          return process_maybe_ident (s, e);
        }

      err_t_wrap (compiler_cpy_advance_expect (s, e), e);
    }

  return SUCCESS; // no more chars to consume
}

static err_t
steady_state_string (compiler *s, error *e)
{
  compiler_assert (s);
  ASSERT (s->state.state == SS_STRING);

  u8 c;

  while (compiler_peek (&c, s))
    {
      if (c == '"')
        {
          compiler_advance_expect (s); // Skip over (last) quote
          return process_string_or_ident (s, TT_STRING, e);
        }

      err_t_wrap (compiler_cpy_advance_expect (s, e), e);
    }

  return SUCCESS;
}

static inline err_t
process_dec (compiler *s, error *e)
{
  compiler_assert (s);

  const string literal = (string){
    .data = s->str,
    .len = s->slen,
  };
  s->slen = 0;

  // Parse the float
  f32 dest;
  err_t_wrap (parse_f32_expect (&dest, literal, e), e);

  err_t_wrap (compiler_process_token (s, tt_float (dest), e), e);

  return SUCCESS;
}

static err_t
steady_state_dec (compiler *s, error *e)
{
  compiler_assert (s);
  ASSERT (s->state.state == SS_DECIMAL);

  u8 c;
  while (compiler_peek (&c, s))
    {
      if (!is_num (c))
        {
          return process_dec (s, e);
        }
      err_t_wrap (compiler_cpy_advance_expect (s, e), e);
    }

  return SUCCESS;
}

static inline err_t
process_int (compiler *s, error *e)
{
  compiler_assert (s);

  const string literal = (string){
    .data = s->str,
    .len = s->slen,
  };
  s->slen = 0;

  // Parse the int
  i32 dest;
  err_t_wrap (parse_i32_expect (&dest, literal, e), e);

  err_t_wrap (compiler_process_token (s, tt_integer (dest), e), e);

  return SUCCESS;
}

static err_t
steady_state_number (compiler *s, error *e)
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
              err_t_wrap (compiler_cpy_advance_expect (s, e), e);
              return steady_state_dec (s, e);
            }
          else
            {
              return process_int (s, e);
            }
        }
      err_t_wrap (compiler_cpy_advance_expect (s, e), e);
    }

  return SUCCESS;
}

static err_t
steady_state_start (compiler *s, error *e)
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
        err_t_wrap (compiler_process_token (s, quick_tok (TT_SEMICOLON), e), e);
        break;
      }
    case '[':
      {
        compiler_advance_expect (s);
        err_t_wrap (compiler_process_token (s, quick_tok (TT_LEFT_BRACKET), e), e);
        break;
      }
    case ']':
      {
        compiler_advance_expect (s);
        err_t_wrap (compiler_process_token (s, quick_tok (TT_RIGHT_BRACKET), e), e);
        break;
      }
    case '{':
      {
        compiler_advance_expect (s);
        err_t_wrap (compiler_process_token (s, quick_tok (TT_LEFT_BRACE), e), e);
        break;
      }
    case '}':
      {
        compiler_advance_expect (s);
        err_t_wrap (compiler_process_token (s, quick_tok (TT_RIGHT_BRACE), e), e);
        break;
      }
    case '(':
      {
        compiler_advance_expect (s);
        err_t_wrap (compiler_process_token (s, quick_tok (TT_LEFT_PAREN), e), e);
        break;
      }
    case ')':
      {
        compiler_advance_expect (s);
        err_t_wrap (compiler_process_token (s, quick_tok (TT_RIGHT_PAREN), e), e);
        break;
      }
    case ',':
      {
        compiler_advance_expect (s);
        err_t_wrap (compiler_process_token (s, quick_tok (TT_COMMA), e), e);
        break;
      }
    case '"':
      {
        s->slen = 0;
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
            s->slen = 0;
            err_t_wrap (compiler_cpy_advance_expect (s, e), e);
            s->state.state = SS_IDENT;
            break;
          }
        else if (is_num (next) || next == '+' || next == '-')
          {
            /**
             * Advance forward once because steady state doesn't
             * account for + or -
             */
            s->slen = 0;
            err_t_wrap (compiler_cpy_advance_expect (s, e), e);
            s->state.state = SS_NUMBER;
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
void
compiler_create (compiler *dest, query_provider *qp)
{
  dest->state.state = SS_START;
  dest->input = cbuffer_create_from (dest->_input);
  dest->output = cbuffer_create_from (dest->_output);
  dest->slen = 0;
  dest->sp = 0;
  dest->parser_work = lalloc_create_from (dest->_parser_work);
  dest->qp = qp;

  compiler_assert (dest);
}

void
compiler_execute_all (compiler *s)
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

      /**
       * Write a maximum of 1 token
       * or else handle error
       */
      if (steady_state_execute (s, NULL))
        {
          // TODO - handle errors
          panic ();
        }
    }
}
