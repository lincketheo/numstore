#include "compiler/stack_parser/stack_parser.h"
#include "compiler/stack_parser/common.h"
#include "compiler/stack_parser/query_builder.h"
#include "compiler/stack_parser/type_builder.h"
#include "compiler/tokens.h"
#include "dev/errors.h"
#include "intf/mm.h"
#include "utils/bounds.h"

////////////////////// TYPE PARSER

DEFINE_DBG_ASSERT_I (stack_parser, stack_parser, t)
{
  ASSERT (t);
  ASSERT (t->stack);
}

err_t
stackp_create (stack_parser *dest, sp_params params)
{
  ASSERT (dest);

  // Allocate the stack - start with 3 layers
  ast_builder *stack = lmalloc (
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

  return SUCCESS;
}

static inline sb_feed_t
astb_expect_next (ast_builder *b, token t)
{
  switch (b->type)
    {
    case SBBT_TYPE:
      {
        return typeb_expect_next (&b->tb, t);
      }
    case SBBT_QUERY:
      {
        return qb_expect_next (&b->qb);
      }
    default:
      {
        ASSERT (0);
        return 0;
      }
    }
}

static inline stackp_result
astb_accept_token (ast_builder *b, token t, lalloc *alloc)
{
  switch (b->type)
    {
    case SBBT_TYPE:
      {
        return typeb_accept_token (&b->tb, t, alloc);
      }
    case SBBT_QUERY:
      {
        return qb_accept_token (&b->qb, t);
      }
    default:
      {
        ASSERT (0);
        return 0;
      }
    }
}

static inline stackp_result
astb_accept_type (ast_builder *b, type t)
{
  switch (b->type)
    {
    case SBBT_TYPE:
      {
        return typeb_accept_type (&b->tb, t);
      }
    case SBBT_QUERY:
      {
        return qb_accept_type (&b->qb, t);
      }
    default:
      {
        ASSERT (0);
        return 0;
      }
    }
}

static inline stackp_result
astb_build (ast_builder *b, lalloc *alloc)
{
  switch (b->type)
    {
    case SBBT_TYPE:
      {
        return typeb_build (&b->tb, alloc);
      }
    case SBBT_QUERY:
      {
        return qb_build (&b->qb);
      }
    default:
      {
        ASSERT (0);
        return 0;
      }
    }
}

stackp_result
stackp_feed_token (stack_parser *tp, token t)
{
  stack_parser_assert (tp);
  ASSERT (tp->sp > 0);

  ast_builder *top = &tp->stack[tp->sp - 1];
  sb_feed_t exp = astb_expect_next (top, t);

  if (exp == SBFT_TYPE)
    {
      type_builder next;
      next.state = TB_UNKNOWN;
      tp->stack[tp->sp++] = (ast_builder){
        .tb = next,
        .type = SBBT_TYPE,
      };
      top = &tp->stack[tp->sp - 1];
      ASSERT (astb_expect_next (top, t) == SBFT_TOKEN);
    }

  stackp_result ret = astb_accept_token (top, t, tp->type_allocator);

  // Otherwise, the acceptor is done
  // and can be reduced with the previous
  // builders
  //
  // sp == 1 means we're at the base
  while (tp->sp > 1 && ret == SPR_DONE)
    {
      // Pop the top off the stack
      ast_builder top = tp->stack[--tp->sp];
      if ((ret = astb_build (&top, tp->type_allocator)) != SPR_DONE)
        {
          return ret;
        }

      // Merge it with the previous
      ret = astb_accept_type (&tp->stack[tp->sp - 1], top.tb.ret);
    }

  return ret;
}

void
stackp_push (stack_parser *sp, sb_build_type type)
{
  stack_parser_assert (sp);
  ASSERT (sp->sp == 0);

  switch (type)
    {
    case SBBT_TYPE:
      {
        sp->stack[sp->sp++] = (ast_builder){
          .tb = typeb_create (),
          .type = type,
        };
        break;
      }
    case SBBT_QUERY:
      {
        sp->stack[sp->sp++] = (ast_builder){
          .qb = qb_create (),
          .type = type,
        };
        break;
      }
    }
}

ast_result
stackp_pop (stack_parser *sp)
{
  stack_parser_assert (sp);
  ASSERT (sp->sp == 1);

  ast_builder top = sp->stack[--sp->sp];

  stackp_result res = astb_build (&top, sp->type_allocator);
  ASSERT (res == SPR_DONE);

  switch (top.type)
    {
    case SBBT_TYPE:
      {
        return (ast_result){
          .type = top.type,
          .t = top.tb.ret,
        };
      }
    case SBBT_QUERY:
      {
        return (ast_result){
          .type = top.type,
          .q = top.qb.ret,
        };
      }
    default:
      {
        ASSERT (0);
        return (ast_result){ 0 };
      }
    }
}

void
stack_parser_release (stack_parser *t)
{
  stack_parser_assert (t);
  lfree (t->stack_allocator, t->stack);
}
