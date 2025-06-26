#pragma once

#include "compiler/tokens.h"
#include "core/dev/assert.h"
typedef struct expr_s expr;

///////////////////////////////////////
////////////// Section Unary
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

#define unary_operator_ASSERT(op) ASSERT (op == MINUS || op == BANG)

static inline unary
unary_c (expr *e, token_t op)
{
  unary_operator_ASSERT (op);

  return (unary){
    .op = op,
    .e = e,
  };
}

///////////////////////////////////////
////////////// Section Binary
typedef struct
{
  expr *left;
  token_t op;
  expr *right;
} binary;

#define binary_operator_ASSERT(op) \
  ASSERT (op == EQUAL_EQUAL || op == BANG_EQUAL || op == LESS || op == LESS_EQUAL || op == GREATER || op == GREATER_EQUAL || op == PLUS || op == MINUS || op == STAR || op == SLASH);

static inline binary
binary_c (expr *left, token_t op, expr *right)
{
  binary_operator_ASSERT (op);

  return (binary){
    .left = left,
    .op = op,
    .right = right,
  };
}

///////////////////////////////////////
////////////// Section Expression
typedef enum
{
  ET_LITERAL,
  ET_UNARY,
  ET_BINARY,
  ET_GROUPING,
  ET_VARIABLE,
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
expr_literal (literal l)
{
  return (expr){ .type = ET_LITERAL, .l = l };
}

static inline expr
expr_unary (unary u)
{
  return (expr){ .type = ET_UNARY, .u = u };
}

static inline expr
expr_binary (binary b)
{
  return (expr){ .type = ET_BINARY, .b = b };
}

static inline expr
expr_grouping (expr *e)
{
  return (expr){ .type = ET_GROUPING, .g = e };
}

static inline expr
expr_variable (char *ident)
{
  return (expr){ .type = ET_VARIABLE, .v = ident };
}

int fprintln_expr (FILE *ofp, expr *e);
