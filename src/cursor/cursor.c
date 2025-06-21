#include "cursor/cursor.h"

#include "hash_map/hm.h" // hm
#include "intf/types.h"
#include "rptree/rptree.h" // rptree

DEFINE_DBG_ASSERT_I (cursor, cursor, c)
{
  ASSERT (c);
}

static const char *TAG = "Cursor";

struct cursor_s
{
  hm *h; // To retrieve / create variables

  struct
  {
    rptree *r;    // To modify currently loaded variable
    variable var; // Current loaded variable
    bool is_loaded;
  };

  pager *p;
};

cursor *
cursor_open (pager *p, error *e)
{
  cursor *ret = i_malloc (1, sizeof *ret);
  if (ret == NULL)
    {
      error_causef (e, ERR_NOMEM, "%s Failed to allocate cursor", TAG);
      return NULL;
    }

  ret->h = hm_open (p, e);
  if (ret->h == NULL)
    {
      i_free (ret);
      return NULL;
    }

  ret->r = NULL;
  ret->is_loaded = false;
  ret->p = p;

  cursor_assert (ret);
  return ret;
}

void
cursor_close (cursor *c)
{
  cursor_assert (c);
  hm_close (c->h);
  i_free (c);
}

err_t
cursor_create_var (cursor *c, create_query *create, error *e)
{
  cursor_assert (c);
  err_t ret = SUCCESS;

  // Create a new rptree
  c->r = rpt_open (-1, c->p, e);
  if (c->r == NULL)
    {
      return err_t_from (e);
    }
  pgno pg0 = rpt_pg0 (c->r);
  ASSERT (pg0 > 0);

  /**
   * Insert it into the variable hash table
   */
  variable var = {
    .pg0 = pg0,
    .type = create->type,
    .vname = create->vname,
  };

  err_t_wrap (hm_insert (c->h, var, e), e);

  return ret;
}
