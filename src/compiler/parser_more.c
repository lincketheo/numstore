#include <stdio.h>  // TODO
#include <stdlib.h> // TODO

#include "compiler/tokens.h"
#include "core/dev/assert.h"
#include "core/dev/testing.h"
#include "core/ds/cbuffer.h"
#include "core/ds/strings.h"
#include "core/errors/error.h" // TODO
#include "core/intf/logging.h"
#include "core/mm/lalloc.h" // TODO

#include "compiler/parser.h" // TODO
#include "numstore/query/queries/create.h"
#include "numstore/query/query.h"
#include "numstore/query/query_provider.h"
#include "numstore/type/types.h"

extern void *lemon_parseAlloc (void *(*mallocProc) (size_t));

extern void lemon_parseFree (
    void *p,
    void (*freeProc) (void *));

extern void lemon_parse (
    void *yyp,
    int yymajor,
    token yyminor,
    parser_result *ctxt);

extern void lemon_parseTrace (FILE *out, char *zPrefix);

static const char *TAG = "Parser";

err_t
parser_create (parser_result *ret, lalloc *work, lalloc *dest, error *e)
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
    .dest = dest,
    .tnum = 0,
    .e = NULL,
    .ready = false,
  };

  return SUCCESS;
}

query
parser_consume (parser_result *p)
{
  ASSERT (p->ready);
  query ret = p->result;
  lemon_parseFree (p->yyp, free);

  p->tnum = 0;
  p->e = NULL;
  p->ready = false;

  return ret;
}

err_t
parser_parse (parser_result *res, token tok, error *e)
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
parser_write_result (parser *s, query res)
{
  ASSERT (cbuffer_avail (s->output) >= sizeof (res));
  u32 ret = cbuffer_write (&res, sizeof res, 1, s->output);
  ASSERT (ret == 1);
}

// Process an error
static inline void
parser_process_error (parser *s)
{
  if (!s->in_err)
    {
      parser_right_after_error_assert (s);
      s->in_err = true;

      query q = query_error_create (s->e);
      ASSERT (cbuffer_avail (s->output) >= sizeof (q));
      u32 ret = cbuffer_write (&q, sizeof q, 1, s->output);
      ASSERT (ret == 1);

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
      i_log_info ("Parser (Normal State) recieved token: %s\n", tt_tostr (t.type));

      // Pre set up stuff
      switch (t.type)
        {
        case case_OPCODE:
          {
            // TODO - check if we're already in a query
            // Create a new parser if you encounter a query
            parser_err_t_wrap (parser_create (&s->parser, &s->parser_work, t.q.qalloc, &s->e), s);
            break;
          }
          // Forward scanner error
        case TT_ERROR:
          {
            bad_error_assert (&t.e);
            s->e = t.e;
            parser_process_error (s);
            continue;
          }
        default:
          {
            break;
          }
        }

      // Consume the token
      parser_err_t_wrap (parser_parse (&s->parser, t, &s->e), s);

      // Check if parser is done
      if (s->parser.ready)
        {
          parser_err_t_wrap (parser_parse (&s->parser, quick_tok (0), &s->e), s);

          // Consume query
          query next = parser_consume (&s->parser);

          // Reset allocator
          lalloc_reset (&s->parser_work);

          i_log_query (next);

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
  ASSERT (cbuffer_avail (output) % sizeof (query) == 0);

  *dest = (parser){
    .input = input,
    .output = output,

    // parser gets initialized on op codes

    .e = error_create (NULL),
    .parser_work = lalloc_create_from (dest->_parser_work),
  };
}

void
parser_execute (parser *p)
{
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
  query_provider *qspce = query_provider_create (&e);

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
