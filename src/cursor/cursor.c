#include "cursor/cursor.h"
#include "errors/error.h"

DEFINE_DBG_ASSERT_I (cursor, cursor, c)
{
  ASSERT (c);
}

cursor
crsr_create (crsr_params params)
{
  return (cursor){
    .r = rpt_create ((rpt_params){
        .alloc = params.alloc,
        .pager = params.pager,
    }),
    .hm = vfhm_create ((vfhm_params){
        .alloc = params.alloc,
        .pager = params.pager,
    }),
    .is_meta_loaded = false,
  };
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
  err_t_wrap (
      vfhm_insert (
          &c->hm,
          query.vname,
          (vmeta){
              .pgn0 = pg0,
              .type = query.type,
          },
          e),
      e);

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

  err_t_wrap (vfhm_get (&c->hm, &c->meta, vname, e), e);

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
