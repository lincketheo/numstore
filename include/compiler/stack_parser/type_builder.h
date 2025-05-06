#pragma once

#include "compiler/stack_parser/common.h"
#include "compiler/stack_parser/types/enum_builder.h"
#include "compiler/stack_parser/types/primitive_builder.h"
#include "compiler/stack_parser/types/sarray_builder.h"
#include "compiler/stack_parser/types/struct_builder.h"
#include "compiler/stack_parser/types/union_builder.h"

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

struct type_builder_s
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
    struct_builder sb;
    union_builder ub;
    enum_builder eb;
    sarray_builder sab;
  };

  type ret;
};

/**
 * Initialize in the unknown state
 */
type_builder typeb_create (void);

/**
 * Run build on [tb], assuming it has everything it needs
 */
stackp_result typeb_build (type_builder *tb, lalloc *alloc);

/**
 * Check with the top type builder and ask: what thing should I
 * give you next given that the next token is [t]
 */
sb_feed_t typeb_expect_next (const type_builder *tb, token t);

/**
 * Give this type_builder a token
 */
stackp_result typeb_accept_token (type_builder *tb, token t, lalloc *alloc);

/**
 * Give this type_builder a type
 */
stackp_result typeb_accept_type (type_builder *tb, type t);
