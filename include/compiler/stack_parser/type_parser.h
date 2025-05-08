#pragma once

#include "compiler/stack_parser/common.h"
#include "compiler/stack_parser/types/enum.h"
#include "compiler/stack_parser/types/prim.h"
#include "compiler/stack_parser/types/sarray.h"
#include "compiler/stack_parser/types/struct.h"
#include "compiler/stack_parser/types/union.h"

////////////////// TYPE BUILDER
/**
 * T -> p |
 *      struct { i T K } |
 *      union { i T K } |
 *      enum { i I } |
 *      A T
 *
 * A -> [NUM] A | [NUM]
 * K -> \eps | , T K
 * I -> \eps | , i I
 *
 * Note:
 * T = TYPE
 * A = ARRAY_PREFIX
 * K = KEY_VALUE_LIST
 * I = ENUM_KEY_LIST
 */

struct type_parser_s
{

  enum
  {
    TB_UNKNOWN,

    TB_STRUCT,
    TB_UNION,
    TB_ENUM,
    TB_VARRAY,
    TB_SARRAY,
    TB_PRIM,

  } state;

  union
  {
    struct_parser stp;
    union_parser unp;
    enum_parser enp;
    sarray_parser sap;
  };

  type ret;
  lalloc *alloc;
};

/**
 * Initialize in the unknown state
 */
type_parser typep_create (lalloc *alloc);

/**
 * Run build on [tb], assuming it has everything it needs
 */
stackp_result typep_build (type_parser *tb);

/**
 * Check with the top type parser and ask: what thing should I
 * give you next given that the next token is [t]
 */
sb_feed_t typep_expect_next (
    const type_parser *tb,
    token t);

/**
 * Give this type_parser a token
 */
stackp_result typep_accept_token (
    type_parser *tb, token t);

/**
 * Give this type_parser a type
 */
stackp_result typep_accept_type (
    type_parser *tb, type t);
