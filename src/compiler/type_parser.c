#include "compiler/type_parser.h"
#include "compiler/ast_builders/common.h"
#include "compiler/ast_builders/type_builder.h"
#include "compiler/tokens.h"
#include "dev/errors.h"
#include "intf/mm.h"
#include "utils/bounds.h"

////////////////////// TYPE PARSER

DEFINE_DBG_ASSERT_I (type_parser, type_parser, t)
{
  ASSERT (t);
  ASSERT (t->stack);
  ASSERT (t->sp > 0);
}

err_t
tp_create (type_parser *dest, tp_params params)
{
  ASSERT (dest);

  // Allocate the stack - start with 3 layers, which is pretty normal
  type_builder *stack = lmalloc (
      params.stack_allocator,
      3 * sizeof *stack);

  if (!stack)
    {
      return ERR_NOMEM;
    }

  dest->stack = stack;
  dest->sp = 0;

  dest->type_allocator = params.type_allocator;
  dest->stack_allocator = params.stack_allocator;

  dest->stack[dest->sp++] = typeb_create ();

  return SUCCESS;
}

parse_result
tp_feed_token (type_parser *tp, token t)
{
  type_builder *top = &tp->stack[tp->sp - 1];

  // What does the top expect?
  parse_expect exp = typeb_expect_next (top, t);

  if (exp == PE_TYPE)
    {
      type_builder next;
      next.state = TB_UNKNOWN;
      tp->stack[tp->sp++] = next;
      top = &tp->stack[tp->sp - 1];
      ASSERT (typeb_expect_next (top, t) == PE_TOKEN);
    }

  parse_result ret = typeb_accept_token (top, t, tp->type_allocator);

  // Otherwise, the acceptor is done
  // and can be reduced with the previous
  // builders
  //
  // sp == 1 means we're at the base
  while (tp->sp > 1 && ret == PR_DONE)
    {
      // Pop the top off the stack
      type_builder top = tp->stack[--tp->sp];
      if ((ret = typeb_build (&top, tp->type_allocator)) != PR_DONE)
        {
          return ret;
        }

      // Merge it with the previous
      ret = typeb_accept_type (&tp->stack[tp->sp - 1], top.ret);
    }

  return ret;
}

void
type_parser_release (type_parser *t)
{
  type_parser_assert (t);
  lfree (t->stack_allocator, t->stack);
}
