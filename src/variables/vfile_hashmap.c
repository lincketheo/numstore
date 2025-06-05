#include "variables/vfile_hashmap.h"

#include "errors/error.h"
#include "mm/lalloc.h"
#include "paging/page.h"
#include "paging/pager.h"
#include "variables/vhash_fmt.h" // vread_hash_fmt

DEFINE_DBG_ASSERT_I (vfile_hashmap, vfile_hashmap, v)
{
  ASSERT (v);
  ASSERT (v->pager);
}
