#include "hash_map/hm.h"

#include "ast/type/types.h"
#include "dev/testing.h"
#include "ds/strings.h"
#include "errors/error.h"
#include "hash_map/hl.h"
#include "intf/io.h"
#include "intf/stdlib.h"
#include "mm/lalloc.h"
#include "paging/page.h"
#include "paging/pager.h"
#include "paging/types/hash_page.h"
#include "variables/variable.h"

struct hm_s
{
  pager *pager;
  hl *hl;
};

DEFINE_DBG_ASSERT_I (hm, hm, v)
{
  ASSERT (v);
  ASSERT (v->pager);
  ASSERT (v->hl);
}

static const char *TAG = "Hash Map";

static err_t
hm_create_first_page (hm *h, error *e)
{
  hm_assert (h);

  const page *_root = pgr_new (h->pager, PG_HASH_PAGE, e);
  if (_root == NULL)
    {
      return err_t_from (e);
    }
  page *root = pgr_make_writable (h->pager, _root);

  ASSERT (root->pg == 0);

  page_init (root, PG_HASH_PAGE);

  return SUCCESS;
}

hm *
hm_open (pager *p, error *e)
{
  // Allocate hash map
  hm *ret = i_malloc (1, sizeof *ret);
  if (ret == NULL)
    {
      error_causef (
          e, ERR_NOMEM,
          "%s: Failed to allocate memory for hash map", TAG);
      return NULL;
    }

  hl *hl = hl_open (p, e);
  if (hl == NULL)
    {
      ASSERT (e->cause_code);
      i_free (ret);
      return NULL;
    }

  // Assign variables
  ret->pager = p;

  // Create first page if non existant
  if (pgr_get_npages (p) == 0)
    {
      err_t_wrap_null (hm_create_first_page (ret, e), e);
    }

  return ret;
}

void
hm_close (hm *h)
{
  hm_assert (h);
  hl_free (h->hl);
  i_free (h);
}

/**
 * Finds the starting leaf for the variable
 * with key name [key].
 *
 * Expects that the value should exist (e.g. you're calling get)
 *
 * Errors:
 *    - ERR_IO / ERR_CORRUPT (pgr_get_expect) - e.g. leaf page
 *      says it exists on page 0 but doesn't or page 0 doesn't
 *      exist
 *    - ERR_DOESNT_EXIST - Leaf doesn't exist
 */
static spgno
hm_fetch_existing_starting_leaf (
    hm *h,
    const string key,
    error *e)
{
  hm_assert (h);

  p_size hpos; // hash index in the hash page
  pgno lpg;    // leaf page number

  /**
   * Fetch the first hash page
   */
  const page *cur = pgr_get (PG_HASH_PAGE, 0, h->pager, e);
  if (cur == NULL)
    {
      ASSERT (e->cause_code);
      return err_t_from (e);
    }

  /**
   * Hash the key
   */
  hpos = hp_get_hash_pos (&cur->hp, key);
  lpg = hp_get_pgno (&cur->hp, hpos);

  /**
   * Hash's of 0 mean the hash position hasn't
   * been indexed before (0 being a magic number)
   */
  if (lpg == 0)
    {
      return error_causef (
          e, ERR_DOESNT_EXIST,
          "Key: %.*s does not exist in the hash table",
          key.len, key.data);
    }

  return lpg;
}

err_t
hm_get (
    hm *h,
    variable *dest,
    lalloc *alloc,
    const string key,
    error *e)
{
  // Center yourself on the starting leaf node
  spgno start = hm_fetch_existing_starting_leaf (h, key, e);
  if (start < 0)
    {
      return (err_t)start;
    }

  var_hash_entry result;

  err_t_wrap (hl_seek (h->hl, &result, start, e), e);

  while (true)
    {
      // Check if we're at the end of the list
      if (hl_eof (h->hl))
        {
          return error_causef (
              e, ERR_DOESNT_EXIST,
              "%s Variable: %.*s doesn't exist",
              TAG, key.len, key.data);
        }

      if (!result.is_tombstone && string_equal (result.vstr, key))
        {
          // Convert to a variable meta data object
          err_t ret = var_hash_entry_deserialize (dest, &result, alloc, e);
          if (ret == ERR_TYPE_DESER)
            {
              // Treat invalid serialize methods as corrupt
              return error_change_causef (
                  e, ERR_CORRUPT,
                  "%s: "
                  "Failed to deserialize type "
                  "for variable: %.*s",
                  TAG, key.len, key.data);
            }
        }

      // Read next
      err_t_wrap (hl_read (h->hl, &result, e), e);
    }
}

/**
 * "Centers" [hl] onto the starting hash leaf page
 *
 * The page doesn't have to exist (e.g. you're calling insert)
 *
 * Errors:
 *    - ERR_IO
 *    - ERR_
 */
static spgno
hm_fetch_starting_leaf (
    hm *h,
    const string key,
    error *e)
{
  hm_assert (h);

  p_size hpos; // Hash index
  pgno lpg;    // Leaf page

  /**
   * Fetch the first hash page
   */
  const page *hp = pgr_get (PG_HASH_PAGE, 0, h->pager, e);
  if (hp == NULL)
    {
      ASSERT (e->cause_code);
      return err_t_from (e);
    }

  /**
   * Hash the key
   */
  hpos = hp_get_hash_pos (&hp->hp, key);
  lpg = hp_get_pgno (&hp->hp, hpos);

  const page *leaf;

  if (lpg == 0)
    {
      /**
       * Create the first leaf page
       */
      leaf = pgr_new (h->pager, PG_HASH_LEAF, e);

      /**
       * Save the starting node of the hash page
       */
      page *hpw = pgr_make_writable (h->pager, hp);
      hp_set_hash (&hpw->hp, hpos, leaf->pg);

      return leaf->pg;
    }
  else
    {
      return lpg;
    }
}

err_t
hm_insert (
    hm *h,
    const variable var,
    error *e)
{
  hm_assert (h);

  // Then create or get the starting hash leaf page for this key
  spgno pg0 = hm_fetch_starting_leaf (h, var.vname, e);
  if (pg0 < 0)
    {
      return (err_t)pg0;
    }

  var_hash_entry result;

  err_t_wrap (hl_seek (h->hl, &result, pg0, e), e);

  while (true)
    {
      // Check if we're at the end of the list
      if (hl_eof (h->hl))
        {
          err_t_wrap (hl_append (h->hl, var, e), e);
          return SUCCESS;
        }

      if (!result.is_tombstone && string_equal (result.vstr, var.vname))
        {

          return error_change_causef (
              e, ERR_ALREADY_EXISTS,
              "%s "
              "Variable: %.*s already exists",
              TAG, var.vname.len, var.vname.data);
        }

      // Read next
      err_t_wrap (hl_read (h->hl, &result, e), e);
    }

  return SUCCESS;
}

err_t
hm_delete (hm *h, const string vname, error *e)
{
  hm_assert (h);

  spgno pg0 = hm_fetch_existing_starting_leaf (h, vname, e);
  if (pg0 < 0)
    {
      return (err_t)pg0;
    }

  var_hash_entry result;

  err_t_wrap (hl_seek (h->hl, &result, pg0, e), e);

  while (true)
    {
      // Check if we're at the end of the list
      if (hl_eof (h->hl))
        {
          return error_causef (
              e, ERR_DOESNT_EXIST,
              "%s Variable: %.*s doesn't exist",
              TAG, vname.len, vname.data);
          return SUCCESS;
        }

      if (!result.is_tombstone && string_equal (result.vstr, vname))
        {
          hl_set_tombstone (h->hl);
          return SUCCESS;
        }

      // Read next
      err_t_wrap (hl_read (h->hl, &result, e), e);
    }

  return SUCCESS;
}

TEST (hash_map)
{
  // Initialization
  error e = error_create (NULL);
  test_fail_if (i_remove_quiet (unsafe_cstrfrom ("test.db"), &e));

  // Create the pager
  pager *p = pgr_open (unsafe_cstrfrom ("test.db"), &e);
  test_fail_if_null (p);

  // Create the hash table
  hm *h = hm_open (p, &e);
  test_fail_if_null (h);

  const variable foo = {
    .pg0 = 123,
    .type = (type){
        .type = T_STRUCT,
        .st = (struct_t){
            .types = (type[]){
                (type){
                    .type = T_PRIM,
                    .p = U64,
                },
                (type){
                    .type = T_PRIM,
                    .p = U32,
                },
                (type){
                    .type = T_PRIM,
                    .p = U8,
                },
            },
            .keys = (string[]){
                (string){
                    .data = "foo",
                    .len = 3,
                },
                (string){
                    .data = "bar",
                    .len = 3,
                },
                (string){
                    .data = "biz",
                    .len = 3,
                },
            },
            .len = 3,
        },
    },
    .vname = (string){
        .data = "a",
        .len = 1,
    },
  };

  (void)foo;
  test_assert_equal (hm_insert (h, foo, &e), SUCCESS);

  // Clean up
  test_fail_if (i_remove_quiet (unsafe_cstrfrom ("test.db"), &e));

  // Create the pager
  test_fail_if (pgr_close (p, &e));

  // Create the hash table
  hm_close (h);

  test_fail_if_null (h);
}
