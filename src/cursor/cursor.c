#include "cursor/cursor.h"
#include "cursor/btree_cursor.h"
#include "dev/errors.h"
#include "intf/stdlib.h"

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
  if ((ret = nav_create (&dest->nav, params.nparams)))
    {
      return ret;
    }

  dest->loaded = false;

  dest->input = params.input;
  dest->output = params.output;
  dest->variables = params.variables;

  cursor_assert (dest);

  return ret;
}

err_t
crsr_create_var (cursor *c, const vcreate v)
{
  cursor_assert (c);

  err_t ret = SUCCESS;

  // First check if exists
  switch ((ret = vhash_map_get (NULL, c->variables, v.vname)))
    {
    case SUCCESS:
      return ERR_ALREADY_EXISTS;
    case ERR_DOESNT_EXIST:
      break;
    default:
      return ret;
    }

  // Fetch the size of the variable type
  // TODO - I think this function should not return
  // errors for internal types - All sizes inside must
  // valid
  u64 size;
  if ((ret = type_bits_size (&size, &v.type)))
    {
      return ret;
    }

  // Create a new index page
  u64 pgno;
  if ((ret = nav_new_root (&c->nav, &pgno, size)))
    {
      return ret;
    }

  // Finally, insert the new variable
  const vmeta meta = {
    .pgn0 = pgno,
    .type = v.type,
  };

  if ((ret = vhash_map_insert (c->variables, v.vname, meta)))
    {
      // TODO - rollback new page
      return ret;
    }

  return SUCCESS;
}

err_t
crsr_load_var_str (cursor *c, const string vname)
{
  cursor_assert (c);

  err_t ret = SUCCESS;

  // Fetch the variable meta information from the hash map
  vmeta meta;
  if ((ret = vhash_map_get (&meta, c->variables, vname)))
    {
      return ret;
    }

  // Fetch the size of the variable type
  // TODO - I think this function should not return
  // errors for internal types - All sizes inside must
  // valid
  u64 size;
  if ((ret = type_bits_size (&size, &meta.type)))
    {
      return ret;
    }

  // Rewind to that new variable
  if ((ret = nav_rewind (&c->nav, meta.pgn0, size)))
    {
      return ret;
    }

  i_memcpy (&c->meta, &meta, sizeof meta);
  c->loaded = true;

  return SUCCESS;
}

err_t
crsr_unload_var_str (cursor *c)
{
  cursor_assert (c);

  if (!c->loaded)
    {
      return ERR_NOT_LOADED;
    }

  c->loaded = false;

  return SUCCESS;
}

err_t
crsr_navigate (cursor *c, u64 toloc)
{
  cursor_assert (c);

  err_t ret;

  if (!c->loaded)
    {
      return ERR_NOT_LOADED;
    }

  if ((ret = nav_navigate (&c->nav, toloc)))
    {
      return ret;
    }

  return SUCCESS;
}
*/

/**
 * Returns:
 */
err_t crsr_read (cursor *c, u64 n, u64 step);

/**
 * Returns:
 */
err_t crsr_write (cursor *c, u64 n);

/**
 * Returns:
 */
err_t crsr_update (cursor *c, u64 n, u64 step);

/**
 * Returns:
 */
err_t crsr_delete (cursor *c, u64 n, u64 step);
