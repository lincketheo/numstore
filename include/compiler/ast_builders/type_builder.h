#pragma once

#include "compiler/ast_builders/common.h"
#include "compiler/ast_builders/types/enum_builder.h"
#include "compiler/ast_builders/types/primitive_builder.h"
#include "compiler/ast_builders/types/sarray_builder.h"
#include "compiler/ast_builders/types/struct_builder.h"
#include "compiler/ast_builders/types/union_builder.h"
#include "compiler/ast_builders/types/varray_builder.h"

////////////////// TYPE BUILDER
/**
 * T -> p |
 *      struct { i T K } |
 *      union { i T K } |
 *      enum { i I } |
 *      A T
 *
 * A -> V | S
 * K -> \eps | , T K
 * V -> [] V | []
 * S -> [NUM] S | [NUM]
 * I -> \eps | , i I
 *
 * Note:
 * T = TYPE
 * A = ARRAY_PREFIX
 * K = KEY_VALUE_LIST
 * V = VARRAY_BRACKETS
 * S = SARRAY_BRACKETS
 * I = ENUM_KEY_LIST
 */

struct type_builder_s
{

  enum
  {
    TB_UNKNOWN,
    TB_ARRAY_UNKNOWN,

    TB_STRUCT,
    TB_UNION,
    TB_ENUM,
    TB_VARRAY,
    TB_SARRAY,
    TB_PRIM,

  } state;

  union
  {
    struct_builder sb;
    union_builder ub;
    enum_builder eb;
    varray_builder vab;
    sarray_builder sab;
  };

  type ret;
};

type_builder typeb_create (void);
parse_result typeb_accept_token (type_builder *tb, token t, lalloc *alloc);
parse_result typeb_accept_type (type_builder *tb, type t);
parse_result typeb_build (type_builder *tb, lalloc *alloc);
parse_expect typeb_expect_next (const type_builder *tb, token t);
