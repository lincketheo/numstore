#include "compiler/stack_parser/common.h"
#include "compiler/stack_parser/type_parser.h"
#include "dev/assert.h"

//////////////// PRIM BUILDER

stackp_result
prim_create (type_parser *tb, prim_t p)
{
  ASSERT (tb);
  ASSERT (tb->state == TB_UNKNOWN);

  tb->ret.p = p;
  tb->ret.type = T_PRIM;
  tb->state = TB_PRIM;

  return SPR_DONE;
}
