#include "cursor/cursor.h"

#include "ast/query/query.h"
#include "errors/error.h"
#include "mm/lalloc.h"
#include "rptree/rptree.h"
#include "variables/vfile_hashmap.h"

DEFINE_DBG_ASSERT_I (cursor, cursor, c)
{
  ASSERT (c);
}

cursor
crsr_open (pager *p)
{
  cursor ret = {
    .hm = vfhm_create (p),
    .is_loaded = false,
    .p = p,
  };

  cursor_assert (&ret);
  return ret;
}

err_t
crsr_create_hash_table (cursor *c, error *e)
{
  cursor_assert (c);
  return vfhm_create_hashmap (&c->hm, e);
}

err_t
crsr_create_var (cursor *c, create_query *create, error *e)
{
  cursor_assert (c);
  err_t ret = SUCCESS;
  pgno pg0;

  /**
   * Create a new rptree
   */
  err_t_wrap (rpt_open_new (&c->r, &pg0, c->p, e), e);

  /**
   * Insert it into the variable hash table
   */
  variable var = {
    .pg0 = pg0,
    .type = create->type,
    .vname = create->vname,
  };

  err_t_wrap (vfhm_insert (&c->hm, var, &create->alloc, e), e);

  return ret;
}
