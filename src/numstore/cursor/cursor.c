#include "numstore/cursor/cursor.h"
#include "compiler/ast/query.h"
#include "core/dev/assert.h"
#include "core/ds/cbuffer.h"
#include "core/errors/error.h"
#include "core/intf/types.h"
#include "core/mm/lalloc.h"
#include "numstore/hash_map/hm.h"
#include "numstore/rptree/rptree.h"
#include "numstore/type/types.h"
#include "numstore/variables/variable.h"

static const char *TAG = "Cursor";

typedef enum
{
  CS_INSERT,
  CS_IDLE,
} cursor_state;

struct cursor_s
{
  hm *h; // To retrieve / create variables
  pager *p;

  // Resumable state
  cursor_state state;
  union
  {
    struct
    {
      rptree *r;
      variable var;
      b_size start;
      b_size len;
      cbuffer *source;
      u8 _var_space[2048];
      lalloc var_space;
    } insert;
  };
};

DEFINE_DBG_ASSERT_I (cursor, cursor, c)
{
  ASSERT (c);
  ASSERT (c->h);
  ASSERT (c->p);
}

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

  ret->p = p;
  ret->state = CS_IDLE;

  cursor_assert (ret);
  return ret;
}

void
cursor_close (cursor *c)
{
  cursor_assert (c);
  switch (c->state)
    {
    case CS_INSERT:
      {
        rpt_close (c->insert.r);
        break;
      }
    case CS_IDLE:
      {
        break;
      }
    }
  hm_close (c->h);
  i_free (c);
}

err_t
cursor_create (
    cursor *c,
    string vname,
    type type,
    error *e)
{
  cursor_assert (c);
  ASSERT (c->state == CS_IDLE);

  // Create a new rptree
  pgno pg0;
  {
    rptree *r = rpt_open (-1, c->p, e);
    if (r == NULL)
      {
        return err_t_from (e);
      }
    pg0 = rpt_pg0 (r);
    ASSERT (pg0 > 0);

    rpt_close (r);
  }

  /**
   * Insert it into the variable hash table
   */
  {
    variable var = {
      .pg0 = pg0,
      .type = type,
      .vname = vname,
    };

    err_t_wrap (hm_insert (c->h, var, e), e);
  }

  return err_t_from (e);
}

err_t
cursor_delete (
    cursor *c,
    string vname,
    error *e)
{
  cursor_assert (c);
  ASSERT (c->state == CS_IDLE);

  // TODO - delete rptree

  err_t_wrap (hm_delete (c->h, vname, e), e);

  return err_t_from (e);
}

// INSERT
err_t
cursor_insert (
    cursor *c,
    string vname,
    b_size start,
    b_size len,
    cbuffer *source,
    error *e)
{
  cursor_assert (c);
  ASSERT (c->state == CS_IDLE);

  variable var;
  c->insert.var_space = lalloc_create_from (c->insert._var_space);

  err_t_wrap (hm_get (c->h, &var, &c->insert.var_space, vname, e), e);

  t_size stride = type_byte_size (&var.type);

  rptree *r = rpt_open (var.pg0, c->p, e);
  if (r == NULL)
    {
      return err_t_from (e);
    }

  c->insert.r = r;
  c->insert.var = var;
  c->insert.start = start * stride;
  c->insert.len = len * stride;
  c->insert.source = source;
  c->state = CS_INSERT;

  return SUCCESS;
}

static err_t
cursor_insert_execute (cursor *c, error *e)
{
  cursor_assert (c);

  // Read into a temporary packet - TODO - can we do this with 0 memcpy's?
  u8 packet[2048];
  u32 read = cbuffer_read (packet, 1, 2048, c->insert.source);

  ASSERT (read <= c->insert.len);

  err_t_wrap (rpt_seek (c->insert.r, c->insert.start, e), e);
  sb_size written = rpt_insert (packet, read, c->insert.r, e);
  if (written < 0)
    {
      return (err_t)written;
    }

  c->insert.len -= written;
  c->insert.start += written;

  // Finished
  if (c->insert.len == 0)
    {
      pgno pg0 = rpt_pg0 (c->insert.r);
      if (pg0 != c->insert.var.pg0)
        {
          c->insert.var.pg0 = pg0;
          err_t_wrap (hm_delete (c->h, c->insert.var.vname, e), e);
          err_t_wrap (hm_insert (c->h, c->insert.var, e), e);
        }
      rpt_close (c->insert.r);
      c->state = CS_IDLE;
    }

  return SUCCESS;
}

err_t
cursor_execute (cursor *c, error *e)
{
  cursor_assert (c);
  ASSERT (c->state != CS_IDLE);

  switch (c->state)
    {
    case CS_INSERT:
      {
        return cursor_insert_execute (c, e);
      }
    default:
      {
        UNREACHABLE ();
      }
    }
}

bool
cursor_idle (cursor *c)
{
  cursor_assert (c);
  return c->state == CS_IDLE;
}
