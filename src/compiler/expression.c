#include "compiler/expression.h"
#include "compiler/tokens.h"
#include "compiler/value/value.h"
#include "core/dev/assert.h"

#include "compiler/expression.h"
#include "compiler/tokens.h"
#include "compiler/value/value.h"
#include "core/dev/assert.h"
#include "core/errors/error.h"

static inline err_t
binary_evaluate (value *dest, binary *b, lalloc *alloc, error *err)
{
  value left, right;

  err_t_wrap (expr_evaluate (&left, b->left, alloc, err), err);
  err_t_wrap (expr_evaluate (&right, b->right, alloc, err), err);

  *dest = left;

  switch (b->op)
    {
    case TT_PLUS:
      {
        return value_plus_value (dest, &right, alloc, err);
      }
    case TT_MINUS:
      {
        return value_minus_value (dest, &right, err);
      }
    case TT_STAR:
      {
        return value_star_value (dest, &right, err);
      }
    case TT_SLASH:
      {
        return value_slash_value (dest, &right, err);
      }

    case TT_EQUAL_EQUAL:
      {
        return value_equal_equal_value (dest, &right, err);
      }
    case TT_BANG_EQUAL:
      {
        return value_bang_equal_value (dest, &right, err);
      }

    case TT_GREATER:
      {
        return value_greater_value (dest, &right, err);
      }
    case TT_GREATER_EQUAL:
      {
        return value_greater_equal_value (dest, &right, err);
      }
    case TT_LESS:
      {
        return value_less_value (dest, &right, err);
      }
    case TT_LESS_EQUAL:
      {
        return value_less_equal_value (dest, &right, err);
      }

    default:
      {
        UNREACHABLE ();
      }
    }
}

static inline err_t
unary_evaluate (value *dest, unary *u, lalloc *alloc, error *err)
{
  value operand;
  err_t_wrap (expr_evaluate (&operand, u->e, alloc, err), err);

  *dest = operand;

  switch (u->op)
    {
    case TT_MINUS:
      {
        return value_minus (dest, err);
      }
    case TT_BANG:
      {
        value_bang (dest);
        return SUCCESS;
      }
    default:
      {
        UNREACHABLE ();
      }
    }
}

err_t
expr_evaluate (value *dest, expr *ex, lalloc *alloc, error *err)
{
  switch (ex->type)
    {
    case ET_BINARY:
      {
        return binary_evaluate (dest, &ex->b, alloc, err);
      }
    case ET_UNARY:
      {
        return unary_evaluate (dest, &ex->u, alloc, err);
      }
    case ET_GROUPING:
      {
        return expr_evaluate (dest, ex->g, alloc, err);
      }
    case ET_VALUE:
      {
        *dest = ex->l;
        return SUCCESS;
      }
    default:
      {
        UNREACHABLE ();
      }
    }
}
