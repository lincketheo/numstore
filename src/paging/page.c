#include "paging/page.h"
#include "dev/assert.h"
#include "errors/error.h"
#include "intf/stdlib.h"
#include "paging/types/data_list.h"
#include "paging/types/hash_leaf.h"
#include "paging/types/hash_page.h"
#include "paging/types/inner_node.h"
#include "utils/bounds.h"

///////////////////////////// PAGE TYPES

#define MIN_PAGE_SIZE 512

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
page_read_expect (
    page *dest,
    int type,
    u8 *raw,
    p_size rlen,
    pgno pg,
    error *e)
{
  ASSERT (dest);
  ASSERT (raw);
  ASSERT (rlen > 0);

  /**
   * Need to read the header in once
   * to check on the unioned type
   */
  pgh header = raw[0];
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
        data_list dl = dl_set_ptrs (raw, rlen);
        err_t_wrap (dl_validate (&dl, e), e);
        dest->dl = dl;
        break;
      }
    case PG_INNER_NODE:
      {
        inner_node in;
        err_t_wrap (in_read_and_set_ptrs (&in, raw, rlen, e), e);
        dest->in = in;
        break;
      }
    case PG_HASH_PAGE:
      {
        hash_page hp = hp_set_ptrs (raw, rlen);
        err_t_wrap (hp_validate (&hp, e), e);
        dest->hp = hp;
        break;
      }
    case PG_HASH_LEAF:
      {
        hash_leaf hl = hl_set_ptrs (raw, rlen);
        err_t_wrap (hl_validate (&hl, e), e);
        dest->hl = hl;
        break;
      }
    default:
      {
        UNREACHABLE ();
      }
    }

  dest->type = type;
  dest->pg = pg;

  return SUCCESS;
}

void
page_init (page *dest, page_type type, u8 *raw, p_size rlen, pgno pg)
{
  ASSERT (dest);

  dest->type = type;
  dest->pg = pg;

  switch (type)
    {
    case PG_DATA_LIST:
      {
        dest->dl = dl_set_ptrs (raw, rlen);
        dl_init_empty (&dest->dl);
        break;
      }
    case PG_INNER_NODE:
      {
        dest->in = in_set_initial_ptrs (raw, rlen);
        in_init_empty (&dest->in);
        break;
      }
    case PG_HASH_PAGE:
      {
        dest->hp = hp_set_ptrs (raw, rlen);
        hp_init_empty (&dest->hp);
        break;
      }
    case PG_HASH_LEAF:
      {
        dest->hl = hl_set_ptrs (raw, rlen);
        hl_init_empty (&dest->hl);
        break;
      }
    default:
      {
        panic ();
      }
    }
}
