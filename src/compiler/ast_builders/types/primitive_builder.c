#include "compiler/ast_builders/common.h"
#include "compiler/ast_builders/type_builder.h"
#include "dev/assert.h"

//////////////// PRIM BUILDER

parse_result
prim_create (type_builder *tb, prim_t p)
{
  ASSERT (tb);
  ASSERT (tb->state == TB_UNKNOWN);

  tb->ret.p = p;
  tb->ret.type = T_PRIM;
  tb->state = TB_PRIM;

  return PR_DONE;
}
