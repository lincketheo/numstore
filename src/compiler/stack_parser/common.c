#include "compiler/stack_parser/common.h"

stackp_result
stckp_res_map (err_t ret, stackp_result next)
{
  switch (ret)
    {
    case ERR_NOMEM:
      {
        return SPR_MALLOC_ERROR;
      }
    case ERR_INVALID_ARGUMENT:
      {
        return SPR_SYNTAX_ERROR;
      }
    case SUCCESS:
      {
        return next;
      }
    default:
      {
        ASSERT (0);
        return SPR_MALLOC_ERROR;
      }
    }
}
