#include "stmtctrl.h"
#include "errors/error.h"
#include "mm/lalloc.h"

void
stmtctrl_create (stmtctrl *dest)
{
  *dest = (stmtctrl){
    .state = STCTRL_EXECTUING,
    ._strs_alloc = lalloc_create_from (dest->_strs),
    .strs_alloc = lalloc_create_from (dest->strs),
    .types_alloc = lalloc_create_from (dest->types),
    .e = error_create (NULL),
  };
}
