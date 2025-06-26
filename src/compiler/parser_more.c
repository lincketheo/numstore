#include <stdio.h>  // TODO
#include <stdlib.h> // TODO

#include "compiler/tokens.h"
#include "core/dev/assert.h"
#include "core/ds/cbuffer.h"
#include "core/errors/error.h" // TODO
#include "core/mm/lalloc.h"    // TODO

#include "compiler/parser.h" // TODO

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
  parser_right_after_error_assert (s);

  if (!s->in_err)
    {
      s->in_err = true;

      // Write to output buffer
      parser_write_result (s, query_error_create (s->e));

      // Reset error
      s->e = error_create (NULL);
    }
  else
    {
      // Swallow error
      i_log_error ("Another error occured while parser was rewinding\n");
      error_log_consume (&s->e);
    }

  parser_steady_state_assert (s);
}

static inline err_t
execute_normal (parser *s)
{
  parser_steady_state_assert (s);
  ASSERT (s->in_err);

  token t;
  while (cbuffer_read (&t, sizeof t, 1, s->input))
    {
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
