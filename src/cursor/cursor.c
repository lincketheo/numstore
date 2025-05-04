#include "cursor/cursor.h"
//#include "cursor/btree_cursor.h"
#include "dev/errors.h"
#include "intf/stdlib.h"
#include "query/queries/create.h"

/**
DEFINE_DBG_ASSERT_I (cursor, cursor, c)
{
  ASSERT (c);
}

err_t
crsr_create (cursor *dest, crsr_params params)
{
  ASSERT (dest);

  err_t ret = SUCCESS;
  btc_create (&dest->btc, params.nparams);

  cursor_assert (dest);

  return ret;
}

err_t
crsr_create_var (cursor *c, const create_query q)
{
  (void)q;
  cursor_assert (c);
  panic ();
}

err_t
crsr_load_var (cursor *c, vmeta *dest, const string vname)
{
  (void)dest;
  (void)vname;
  cursor_assert (c);
  panic ();
}

err_t
crsr_unload_var (cursor *c)
{
  cursor_assert (c);
  panic ();
}
*/
