#pragma once

#include "compiler/tokens.h"
#include "core/dev/assert.h"

typedef struct expr_s expr;

///////////////////////////////////////
////////////// Section Unary

// !EXPR
// -EXPR
typedef struct
{
  token_t op;
  expr *e;
} unary;

DEFINE_DBG_ASSERT_I (unary, unary, u)
{
  ASSERT (u);
  ASSERT (u->op == TT_MINUS || u->op == TT_BANG);
}

///////////////////////////////////////
////////////// Section Binary

// EXPR + EXPR
// EXPR == EXPR
// etc.
typedef struct
{
  expr *left;
  token_t op;
  expr *right;
} binary;

DEFINE_DBG_ASSERT_I (binary, binary, b)
{
  switch (b->op)
    {
    case TT_EQUAL_EQUAL:
    case TT_BANG_EQUAL:
    case TT_LESS:
    case TT_LESS_EQUAL:
    case TT_GREATER:
    case TT_GREATER_EQUAL:
    case TT_PLUS:
    case TT_MINUS:
    case TT_STAR:
    case TT_SLASH:
      break;
    default:
      ASSERT (false);
    }
}

///////////////////////////////////////
////////////// Section Expression

typedef enum
{
  ET_VALUE,
  ET_UNARY,
  ET_BINARY,
  ET_GROUPING,
} expr_t;

struct expr_s
{
  union
  {
    literal l;
    unary u;
    binary b;
    expr *g;
    char *v;
  };
  expr_t type;
};

static inline expr
create_literal_expr (literal l)
{
  return (expr){ .type = ET_VALUE, .l = l };
}

static inline expr
create_grouping_expr (expr *e)
{
  return (expr){ .type = ET_GROUPING, .g = e };
}

static inline expr
create_unary_expr (expr *e, token_t op)
{
  unary ret = {
    .op = op,
    .e = e,
  };

  unary_assert (&ret);

  return (expr){
    .type = ET_UNARY,
    .u = ret,
  };
}

static inline expr
create_binary_expr (expr *left, token_t op, expr *right)
{
  binary ret = {
    .left = left,
    .op = op,
    .right = right,
  };

  binary_assert (&ret);

  return (expr){
    .type = ET_BINARY,
    .b = ret,
  };
}

err_t expr_evaluate (literal *dest, expr *exp, lalloc *work, error *e);
