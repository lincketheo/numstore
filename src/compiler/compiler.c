#include "compiler/compiler.h"

#include "compiler/scanner.h"
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

struct compiler_s
{
  // outputs
  cbuffer chars;
  cbuffer tokens;
  cbuffer queries;

  char _chars[10];
  token _tokens[10];
  query _queries[10];

  scanner s;
  parser p;
};

DEFINE_DBG_ASSERT_I (compiler, compiler, s)
{
  ASSERT (s);
}

static const char *TAG = "Compiler";

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
    .chars = cbuffer_create_from (ret->_chars),
    .tokens = cbuffer_create_from (ret->_tokens),
    .queries = cbuffer_create_from (ret->_queries),
  };

  scanner_init (&ret->s, &ret->chars, &ret->tokens, qp, &ret->p.parser_work);
  parser_init (&ret->p, &ret->tokens, &ret->queries);

  compiler_assert (ret);

  return ret;
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
  return &c->chars;
}

cbuffer *
compiler_get_output (compiler *c)
{
  compiler_assert (c);
  return &c->queries;
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
      if (cbuffer_avail (&s->queries) < sizeof (query))
        {
          return;
        }

      /**
       * Block on upstream
       */
      if (cbuffer_len (&s->chars) < sizeof (char))
        {
          return;
        }

      scanner_execute (&s->s);
      parser_execute (&s->p);
    }
}
