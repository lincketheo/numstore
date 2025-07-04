#include "numstore/paging/page.h"

#include "core/dev/assert.h" // DEFINE_DBG_ASSERT_I

DEFINE_DBG_ASSERT_I (page, page, p)
{
  ASSERT (p);
}

const char *
page_type_tostr (page_type t)
{
  switch (t)
    {
    case PG_DATA_LIST:
      {
        return "Data List";
      }
    case PG_INNER_NODE:
      {
        return "Inner Node";
      }
    case PG_HASH_PAGE:
      {
        return "Hash Root";
      }
    case PG_HASH_LEAF:
      {
        return "Hash Leaf";
      }
    default:
      {
        return "Unknown Page Type";
      }
    }
}

err_t
page_set_ptrs_expect_type (
    page *p,
    int type,
    error *e)
{
  /**
   * Need to read the header in once
   * to check on the unioned type
   */
  pgh header = p->raw[0];
  if (!(header & type))
    {
      return error_causef (
          e, ERR_CORRUPT,
          "Page: Failed to read expected page type");
    }

  switch (header)
    {
    case PG_DATA_LIST:
      {
        dl_set_ptrs (&p->dl, p->raw);
        err_t_wrap (dl_validate (&p->dl, e), e);
        p->type = PG_DATA_LIST;
        return SUCCESS;
      }
    case PG_INNER_NODE:
      {
        err_t_wrap (in_read_and_set_ptrs (&p->in, p->raw, e), e);
        p->type = PG_INNER_NODE;
        return SUCCESS;
      }
    case PG_HASH_PAGE:
      {
        hp_set_ptrs (&p->hp, p->raw);
        err_t_wrap (hp_validate (&p->hp, e), e);
        p->type = PG_HASH_PAGE;
        return SUCCESS;
      }
    case PG_HASH_LEAF:
      {
        hl_set_ptrs (&p->hl, p->raw);
        err_t_wrap (hl_validate (&p->hl, e), e);
        p->type = PG_HASH_LEAF;
        return SUCCESS;
      }
    default:
      {
        UNREACHABLE ();
      }
    }
}

void
page_init (page *p, page_type type)
{
  p->type = type;

  switch (type)
    {
    case PG_DATA_LIST:
      {
        dl_set_ptrs (&p->dl, p->raw);
        dl_init_empty (&p->dl);
        return;
      }
    case PG_INNER_NODE:
      {
        in_set_initial_ptrs (&p->in, p->raw);
        in_init_empty (&p->in);
        return;
      }
    case PG_HASH_PAGE:
      {
        hp_set_ptrs (&p->hp, p->raw);
        hp_init_empty (&p->hp);
        return;
      }
    case PG_HASH_LEAF:
      {
        hl_set_ptrs (&p->hl, p->raw);
        hl_init_empty (&p->hl);
        return;
      }
    default:
      {
        UNREACHABLE ();
      }
    }
}

void
i_log_page (const page *p)
{
  page_assert (p);
  switch (p->type)
    {
    case PG_HASH_PAGE:
      {
        i_log_hp (&p->hp);
        return;
      }
    case PG_DATA_LIST:
      {
        i_log_dl (&p->dl);
        return;
      }
    case PG_INNER_NODE:
      {
        i_log_in (&p->in);
        return;
      }
    case PG_HASH_LEAF:
      {
        i_log_hl (&p->hl);
        return;
      }
    default:
      {
        UNREACHABLE ();
      }
    }
}
