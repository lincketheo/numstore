#include "compiler/ast/expression.h"

static inline err_t
binary_evaluate (literal *dest, binary *b, lalloc *alloc, error *err)
{
  literal left, right;

  err_t_wrap (expr_evaluate (&left, b->left, alloc, err), err);
  err_t_wrap (expr_evaluate (&right, b->right, alloc, err), err);

  *dest = left;

  switch (b->op)
    {
    case TT_PLUS:
      {
        return literal_plus_literal (dest, &right, alloc, err);
      }
    case TT_MINUS:
      {
        return literal_minus_literal (dest, &right, err);
      }
    case TT_STAR:
      {
        return literal_star_literal (dest, &right, err);
      }
    case TT_SLASH:
      {
        return literal_slash_literal (dest, &right, err);
      }
    case TT_EQUAL_EQUAL:
      {
        return literal_equal_equal_literal (dest, &right, err);
      }
    case TT_BANG_EQUAL:
      {
        return literal_bang_equal_literal (dest, &right, err);
      }
    case TT_GREATER:
      {
        return literal_greater_literal (dest, &right, err);
      }
    case TT_GREATER_EQUAL:
      {
        return literal_greater_equal_literal (dest, &right, err);
      }
    case TT_LESS:
      {
        return literal_less_literal (dest, &right, err);
      }
    case TT_LESS_EQUAL:
      {
        return literal_less_equal_literal (dest, &right, err);
      }
    case TT_CARET:
      {
        return literal_caret_literal (dest, &right, err);
      }
    case TT_PERCENT:
      {
        return literal_mod_literal (dest, &right, err);
      }
    case TT_PIPE:
      {
        return literal_pipe_literal (dest, &right, err);
      }
    case TT_PIPE_PIPE:
      {
        literal_pipe_pipe_literal (dest, &right);
        return SUCCESS;
      }
    case TT_AMPERSAND:
      {
        return literal_ampersand_literal (dest, &right, err);
      }
    case TT_AMPERSAND_AMPERSAND:
      {
        literal_ampersand_ampersand_literal (dest, &right);
        return SUCCESS;
      }
    default:
      {
        UNREACHABLE ();
      }
    }
}

static inline err_t
unary_evaluate (literal *dest, unary *u, lalloc *alloc, error *err)
{
  literal operand;
  err_t_wrap (expr_evaluate (&operand, u->e, alloc, err), err);

  *dest = operand;

  switch (u->op)
    {
    case TT_NOT:
      {
        return literal_not (dest, err);
      }
    case TT_MINUS:
      {
        return literal_minus (dest, err);
      }
    case TT_BANG:
      {
        literal_bang (dest);
        return SUCCESS;
      }
    default:
      {
        UNREACHABLE ();
      }
    }
}

err_t
expr_evaluate (literal *dest, expr *ex, lalloc *alloc, error *err)
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
