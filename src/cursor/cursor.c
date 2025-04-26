#include "cursor/cursor.h"
#include "cursor/navigation.h"
#include "dev/errors.h"
#include "intf/stdlib.h"

DEFINE_DBG_ASSERT_I (cursor, cursor, c)
{
  ASSERT (c);
  // TODO
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
  if ((ret = vhash_map_get (NULL, c->variables, v.vname)))
    {
      return ret;
    }

  // Create a new index page
  u64 pgno;
  if ((ret = nav_goto_new_root (&c->nav, &pgno)))
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

  vmeta meta;
  if ((ret = vhash_map_get (&meta, c->variables, vname)))
    {
      return ret;
    }

  u64 size;
  if ((ret = type_bits_size (&size, &meta.type)))
    {
      return ret;
    }

  c->size = size; // TODO
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

  if ((ret = nav_goto_root (&c->nav, c->meta.pgn0)))
    {
      return ret;
    }

  if ((ret = nav_navigate (&c->nav, toloc, c->size)))
    {
      return ret;
    }

  return SUCCESS;
}
