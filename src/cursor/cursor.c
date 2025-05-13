#include "cursor/cursor.h"
#include "errors/error.h"
#include "mm/lalloc.h"
#include "rptree/rptree.h"
#include "variables/vfile_hashmap.h"

DEFINE_DBG_ASSERT_I (cursor, cursor, c)
{
  ASSERT (c);
}

err_t
crsr_create (cursor *dest, crsr_params params, error *e)
{
  ASSERT (dest);

  rpt_params p = {
    .alloc = params.alloc,
    .pager = params.pager,
  };
  err_t_wrap (rpt_create (&dest->r, p, e), e);

  dest->hm = vfhm_create (params.pager);
  dest->is_meta_loaded = false;

  cursor_assert (dest);

  return SUCCESS;
}

err_t
crsr_create_hash_table (cursor *c, error *e)
{
  cursor_assert (c);
  return vfhm_create_hashmap (&c->hm, e);
}

err_t
crsr_create_var (cursor *c, const create_query query, error *e)
{
  cursor_assert (c);
  err_t ret = SUCCESS;
  pgno pg0;

  /**
   * Create the rptree
   */
  err_t_wrap (rpt_new (&pg0, &c->r, e), e);

  /**
   * Insert it into the variable hash table
   */
  vmeta m = {
    .pgn0 = pg0,
    .type = query.type,
  };
  err_t_wrap (vfhm_insert (&c->hm, query.vname, m, e), e);

  return ret;
}

err_t
crsr_create_and_load_var (cursor *c, const create_query query, error *e)
{
  err_t ret = SUCCESS;

  err_t_wrap (crsr_create_var (c, query, e), e);

  err_t_wrap (crsr_load_var (c, query.vname, e), e);

  return ret;
}

err_t
crsr_load_var (cursor *c, const string vname, error *e)
{
  cursor_assert (c);
  ASSERT (!c->is_meta_loaded);
  err_t ret = SUCCESS;

  lalloc dest = lalloc_create (c->tstr, sizeof (c->tstr));
  err_t_wrap (vfhm_get (&c->hm, &c->meta, vname, &dest, e), e);

  c->is_meta_loaded = true;

  return ret;
}

void
crsr_unload_var (cursor *c)
{
  cursor_assert (c);
  ASSERT (c->is_meta_loaded);
  c->is_meta_loaded = false;
}
