#include "compiler/parser.h"

#include "compiler/tokens.h"
#include "core/dev/testing.h"
#include "core/ds/cbuffer.h"
#include "core/mm/lalloc.h"
#include <stdlib.h>

extern void *lemon_parseAlloc (void *(*mallocProc) (size_t));

extern void lemon_parseFree (
    void *p,
    void (*freeProc) (void *));

extern void lemon_parse (
    void *yyp,
    int yymajor,
    token yyminor,
    parser_result *ctxt);

static const char *TAG = "Parser";

err_t
parser_result_create (parser_result *ret, lalloc *work, error *e)
{
  ASSERT (ret);

  void *yyp = lemon_parseAlloc (malloc);
  if (yyp == NULL)
    {
      return error_causef (e, ERR_NOMEM, "%s Failed to allocate lemon parser", TAG);
    }

  *ret = (parser_result){
    .yyp = yyp,
    .work = work,
    .tnum = 0,
    .e = NULL,
    .result = NULL,
    .done = false,
  };

  return SUCCESS;
}

static inline void
parser_result_reset (parser_result *p)
{
  if (p->yyp)
    {
      lemon_parseFree (p->yyp, free);
      p->yyp = NULL;
    }

  lalloc_reset (p->work);
  p->tnum = 0;
  p->e = NULL;
  p->done = false;
}

static inline void
parser_result_invalidate (parser_result *p)
{
  if (p->result)
    {
      statement_free (p->result);
      p->result = NULL;
    }

  parser_result_reset (p);
}

statement *
parser_result_consume (parser_result *p)
{
  ASSERT (p->done);
  ASSERT (p->yyp);

  statement *ret = p->result;
  ASSERT (ret);

  parser_result_reset (p);

  return ret;
}

err_t
parser_result_parse (parser_result *res, token tok, error *e)
{
  res->e = e;
  lemon_parse (res->yyp, tok.type, tok, res);
  err_t ret = err_t_from (res->e);

  if (!ret)
    {
      res->tnum++;
    }
  return ret;
}

//////////////////////////////////////////////////
/// Parser

DEFINE_DBG_ASSERT_I (parser, parser, s)
{
  ASSERT (s);
}

DEFINE_DBG_ASSERT_I (parser, parser_steady_state, s)
{
  parser_assert (s);
  ok_error_assert (&s->e);
}

DEFINE_DBG_ASSERT_I (parser, parser_right_after_error, s)
{
  parser_assert (s);
  bad_error_assert (&s->e);
  ASSERT (s->parser.result == NULL);
}

#define parser_err_t_wrap(expr, c)  \
  do                                \
    {                               \
      err_t __ret = (err_t)expr;    \
      if (__ret < SUCCESS)          \
        {                           \
          parser_process_error (c); \
          return __ret;             \
        }                           \
    }                               \
  while (0)

#define parser_err_t_continue(expr, c) \
  do                                   \
    {                                  \
      err_t __ret = (err_t)expr;       \
      if (__ret < SUCCESS)             \
        {                              \
          parser_process_error (c);    \
        }                              \
    }                                  \
  while (0)

static inline void
parser_write_result (parser *s, statement *stmt)
{
  ASSERT (stmt);
  statement_result result = {
    .stmt = stmt,
    .ok = true,
  };

  cbuffer_write_expect (&result, sizeof (result), 1, s->output);
}

// Process an error
static inline void
parser_process_error (parser *s)
{
  if (!s->in_err)
    {
      parser_right_after_error_assert (s);
      s->in_err = true;

      // Invalidate the parser (and free the statement)
      parser_result_invalidate (&s->parser);

      statement_result result = {
        .e = s->e,
        .ok = false,
      };

      cbuffer_write_expect (&result, sizeof result, 1, s->output);

      // Reset error
      s->e = error_create (NULL);
    }

  parser_steady_state_assert (s);
}

static inline err_t
execute_normal (parser *s)
{
  parser_steady_state_assert (s);
  ASSERT (!s->in_err);

  token t;
  while (cbuffer_read (&t, sizeof t, 1, s->input))
    {
      i_log_trace ("Parser (Normal State) recieved token: %s\n", tt_tostr (t.type));

      switch (t.type)
        {
          // If we encounter an OPCODE
        case case_OPCODE:
          {

#ifndef NDEBUG
            if (s->expect_opcode)
              {
                ASSERT (t.stmt != NULL);
                ASSERT (s->parser.yyp == NULL);
                s->expect_opcode = false;
              }
#endif

            // And statement is non null
            if (t.stmt != NULL)
              {
                ASSERT (s->parser.yyp == NULL); // TODO - I think this is true - prove it

                // Start a new lemon parser
                parser_err_t_wrap (parser_result_create (&s->parser, &s->parser_work, &s->e), s);
              }
            break;
          }
          // Forward scanner error
        case TT_ERROR:
          {
            bad_error_assert (&t.e);

            // Transfer scanner error over to the parser
            s->e = t.e;

            // Forward the error
            parser_process_error (s);

            // We know that the scanner MUST emit a valid op code token
            // So the next token must be valid
            s->in_err = false;

#ifndef NDEBUG
            s->expect_opcode = true; // Next token MUST be an op code with valid statement
#endif

            continue;
          }
        default:
          {
            break;
          }
        }

      // Consume the token
      parser_err_t_wrap (parser_result_parse (&s->parser, t, &s->e), s);

      // Check if parser is done
      if (s->parser.done)
        {
          parser_err_t_wrap (parser_result_parse (&s->parser, quick_tok (0), &s->e), s);

          // Consume query
          statement *next = parser_result_consume (&s->parser);

          // Reset allocator
          lalloc_reset (&s->parser_work);

          // Write to output buffer
          parser_write_result (s, next);
        }
    }

  return SUCCESS;
}

static void
execute_err (parser *s)
{
  parser_steady_state_assert (s);
  ASSERT (s->in_err);

  // Read until you see a semi colon
  token t;
  while (cbuffer_read (&t, sizeof t, 1, s->input))
    {
      i_log_info ("Parser (Error state) recieved token: %s\n", tt_tostr (t.type));
      if (t.type == TT_SEMICOLON)
        {
          s->in_err = false;
          return;
        }
    }
}

static err_t
execute_one_token (parser *p)
{
  parser_steady_state_assert (p);

  if (p->in_err)
    {
      execute_err (p);
      return SUCCESS;
    }
  else
    {
      return execute_normal (p);
    }
}

void
parser_init (
    parser *dest,
    cbuffer *input,
    cbuffer *output)
{
  ASSERT (dest);
  ASSERT (input);
  ASSERT (cbuffer_avail (input) % sizeof (token) == 0);
  ASSERT (output);
  ASSERT (cbuffer_avail (output) % sizeof (statement_result) == 0);

  *dest = (parser){
    .input = input,
    .output = output,
    .parser = { 0 },
    .e = error_create (NULL),
    .parser_work = lalloc_create_from (dest->_parser_work),
    .in_err = false,

#ifndef NDEBUG
    .expect_opcode = false,
#endif
  };
}

void
parser_execute (parser *p)
{
  parser_assert (p);

  while (true)
    {
      /**
       * Block on downstream
       */
      if (cbuffer_avail (p->output) < sizeof (query))
        {
          return;
        }

      /**
       * Block on upstream
       */
      if (cbuffer_len (p->input) < sizeof (token))
        {
          return;
        }

      parser_err_t_continue (execute_one_token (p), p);
    }
}

/**
#ifndef NTEST
void
test_parser_case (const token *input, const query *expected_output, u32 ilen, u32 olen)
{
  parser p;

  token _tokens[20];
  query _queries[10];

  cbuffer tokens = cbuffer_create_from (_tokens);
  cbuffer queries = cbuffer_create_from (_queries);

  parser_init (&p, &tokens, &queries);

  // expected_output i
  u32 ii = 0;
  u32 oi = 0;

  while (true)
    {
      // Seed scanner
      if (ii < ilen)
        {
          ii += cbuffer_write (input + ii, sizeof *input, ilen - ii, &tokens);
        }
      else
        {
          break;
        }

      // Execute scanner
      parser_execute (&p);

      // Check resulting tokens
      query left;
      while (cbuffer_read (&left, sizeof left, 1, &queries))
        {
          test_fail_if (oi > olen);
          query right = expected_output[oi];

          if (!query_equal (&left, &right))
            {
              i_log_failure ("Actual and expected queries do not equal each other\n");
              i_log_failure ("Actual: \n");
              i_log_query (left);
              i_log_failure ("Expected: \n");
              i_log_query (right);
            }
          test_assert (query_equal (&left, &right));
          oi++;
        }
    }
}
#endif

TEST (parser)
{
  error e = error_create (NULL);

  query actual;
  query expected;

  test_fail_if_null (qspce);
  test_fail_if (query_provider_get (qspce, &actual, QT_CREATE, &e));
  test_fail_if (query_provider_get (qspce, &expected, QT_CREATE, &e));

  expected.create->vname = unsafe_cstrfrom ("foo");
  expected.create->type = (type){
    .type = T_PRIM,
    .p = U32,
  };

  const token tk1[] = {
    tt_opcode (TT_CREATE, actual),
    tt_ident (unsafe_cstrfrom ("foo")),
    tt_prim (U32),
    quick_tok (TT_SEMICOLON),
  };

  test_parser_case (tk1, (query[]){ expected }, 4, 1);
}
*/
