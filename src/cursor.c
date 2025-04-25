#include "cursor.h"
#include "database.h"
#include "dev/assert.h"
#include "dev/errors.h"
#include "intf/stdlib.h"
#include "navigation.h"
#include "paging.h"
#include "sds.h"
#include "typing.h"
#include "vhash_map.h"

DEFINE_DBG_ASSERT_I (cursor, cursor, c)
{
  ASSERT (c);
  // TODO
}

err_t
crsr_create (cursor *dest, database *db)
{
  ASSERT (dest);

  err_t ret = SUCCESS;
  if ((ret = nav_create (&dest->nav, db)))
    {
      return ret;
    }

  dest->loaded = false;
  dest->input = NULL;
  dest->output = NULL;

  cursor_assert (dest);

  return ret;
}

err_t
crsr_rewind (cursor *c, const string vname)
{
  cursor_assert (c);

  err_t ret;

  vmeta dest;
  u64 size;

  if ((ret = vhash_map_get (&dest, c->variables, vname)))
    {
      return ret;
    }

  if ((ret = type_bits_size (&size, &dest.type)))
    {
      return ret;
    }

  if ((ret = nav_goto_page (&c->nav, dest.pgn0, PG_INNER_NODE)))
    {
      return ret;
    }

  i_memcpy (&c->meta, &dest, sizeof (dest));

  return SUCCESS;
}

err_t
crsr_create_variable (cursor *c, const vcreate v)
{
  cursor_assert (c);

  err_t ret = SUCCESS;

  // First check if exists
  if ((ret = vhash_map_get (NULL, c->variables, v.vname)))
    {
      return ret;
    }

  // Then create and go to new page
  if ((ret = nav_goto_new_page (&c->nav, PG_INNER_NODE)))
    {
      return ret;
    }

  // Finally, insert the new variable
  const vmeta meta = {
    .pgn0 = nav_top (&c->nav)->page.pgno,
    .type = v.type,
  };

  if ((ret = vhash_map_insert (c->variables, v.vname, meta)))
    {
      // TODO - rollback new page
      return ret;
    }

  return SUCCESS;
}
