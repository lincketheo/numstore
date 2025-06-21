#include "hash_map/hm.h"
#include "ast/type/types.h"
#include "dev/testing.h"
#include "ds/strings.h"
#include "errors/error.h"
#include "intf/io.h"
#include "mm/lalloc.h"
#include "paging/page.h"
#include "paging/pager.h"
#include "paging/types/hash_page.h"
#include "variables/vhash_fmt.h"

struct hm_s
{
  pager *pager;
};

DEFINE_DBG_ASSERT_I (hm, hm, v)
{
  ASSERT (v);
  ASSERT (v->pager);
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
static const page *
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
      return NULL;
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
      error_causef (
          e, ERR_DOESNT_EXIST,
          "Key: %.*s does not exist in the hash table",
          key.len, key.data);
      return NULL;
    }

  /**
   * Fetch the first leaf page
   */
  return pgr_get (PG_HASH_LEAF, lpg, h->pager, e);
}

static err_t
hm_scroll_and_find_key (
    hm *h,
    vread_hash_fmt *dest, // result
    const page *start,    // Starting page
    const string key,     // Key we're looking for
    error *e              // Any errors along the way
)
{
  p_size pos = 0; // The byte position in the current hash list
  const page *cur = start;

  /**
   * TODO - what if you have a cycle in the linked list?
   * (it would be invalid)
   */
  while (true)
    {
      ASSERT (pos <= HL_DATA_LEN);

      switch (dest->state)
        {

        case VHFMT_CORRUPT:
          {
            /**
             * You can't get here
             * because putting it in an invalid
             * state would be caught by our previous
             * error handlers inside vrhfmt
             */
            UNREACHABLE ();
          }

        case VHFMT_EOF:
          {
            return error_causef (
                e, ERR_DOESNT_EXIST,
                "Variable: %.*s doesn't exist",
                key.len, key.data);
          }

        case VHFMT_DONE:
          {
            if (!dest->is_tombstone)
              {
                string vstr = (string){
                  .data = dest->vstr,
                  .len = dest->vstrlen,
                };
                if (string_equal (vstr, key))
                  {
                    return SUCCESS;
                  }
              }
            break;
          }

        default:
          {
            /**
             * We need to fetch a new page
             */
            if (pos == HL_DATA_LEN)
              {
                pgno next = hl_get_next (&cur->hl);
                if (next == 0)
                  {
                    /*
                     * Deserializer handles EOF, not paging
                     * so this is corrupt because there should be
                     * more
                     */
                    return error_causef (
                        e, ERR_CORRUPT,
                        "Hash Leaf expecting next page");
                  }

                cur = pgr_get (PG_HASH_LEAF, next, h->pager, e);
                if (cur == NULL)
                  {
                    return err_t_from (e);
                  }

                pos = 0;
              }

            // Do the read
            p_size nbytes = HL_DATA_LEN - pos;
            err_t_wrap (vrhfmt_read_in (cur->hl.data + pos, &nbytes, dest, e), e);
            pos += nbytes;

            ASSERT (nbytes > 0); // infinite loops are impossible
          }
        }
    }
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
  const page *cur = hm_fetch_existing_starting_leaf (h, key, e);
  if (cur == NULL)
    {
      return err_t_from (e);
    }

  // Scroll leafs until you find yours
  vread_hash_fmt entry = vrhfmt_create ();
  err_t_wrap (hm_scroll_and_find_key (h, &entry, cur, key, e), e);

  ASSERT (entry.state == VHFMT_DONE);

  // Convert to a more friendly format
  var_hash_entry result = vrhfmt_consume (&entry);

  // Convert to a variable meta data object
  err_t ret = var_hash_entry_deserialize (dest, &result, alloc, e);
  if (ret == ERR_TYPE_DESER)
    {
      // Treat invalid serialize methods as corrupt
      return error_change_causef (
          e, ERR_CORRUPT,
          "VHash Map: "
          "Failed to deserialize type "
          "for variable: %.*s",
          key.len, key.data);
    }
  return ret;
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
static const page *
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
      return NULL;
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
    }
  else
    {
      /**
       * Otherwise fetch the first page
       */
      leaf = pgr_get (PG_HASH_LEAF, lpg, h->pager, e);
      if (leaf == NULL)
        {
          return NULL;
        }
    }

  return leaf;
}

/**
 * Scrolls to the end of this variable hash leaf page
 */
static err_t
vfhmi_scroll_to_eof (
    const page *leaf,  // The starting leaf
    p_size *dest_ppos, // The local page pos at the end
    pager *p,          // Used to fetch pages
    const string key,  // Used to check for dupes
    error *e)
{
  ASSERT (leaf->type == PG_HASH_LEAF);

  /**
   * Create a deserializer to scan the variables
   * first to find duplicates or the end (very similar
   * to the get routine)
   */
  vread_hash_fmt v = vrhfmt_create (); // The reader
  p_size pos = 0;                      // Local position on page

  while (v.state != VHFMT_EOF)
    {
      ASSERT (pos <= HL_DATA_LEN);

      switch (v.state)
        {

        case VHFMT_CORRUPT:
          {
            UNREACHABLE ();
            return ERR_FALLBACK;
          }
        case VHFMT_EOF:
          {
            UNREACHABLE (); // Checked in loop condition
            break;
          }
        case VHFMT_DONE:
          {
            if (!v.is_tombstone)
              {
                /**
                 * Compare strings to see if there's
                 * already a match
                 */
                string vstr = (string){
                  .data = v.vstr,
                  .len = v.vstrlen,
                };
                bool equal = string_equal (vstr, key);

                if (equal)
                  {
                    return error_causef (
                        e, ERR_ALREADY_EXISTS,
                        "Variable: "
                        "%.*s already exists",
                        key.len, key.data);
                  }
              }
            break;
          }
        default:
          {
            /**
             * Reached the end of this page,
             * read the next page
             */
            if (pos == HL_DATA_LEN)
              {
                pgno next = hl_get_next (&leaf->hl);
                if (next == 0)
                  {
                    /**
                     * Deserializer handles EOF, not paging
                     * so this is corrupt because there should be
                     * more
                     */
                    return error_causef (
                        e, ERR_CORRUPT,
                        "Hash Leaf expecting next page");
                  }

                leaf = pgr_get (PG_HASH_LEAF, next, p, e);
                pos = 0; // Reset to the start of the page
              }

            /**
             * Read into vrhfmt
             */
            p_size nbytes = HL_DATA_LEN - pos;
            err_t_wrap (vrhfmt_read_in (
                            leaf->hl.data + pos,
                            &nbytes, &v, e),
                        e);
            pos += nbytes;

            ASSERT (nbytes > 0); // Avoid infinite loops
          }
        }
    }

  ASSERT (v.state == VHFMT_EOF);
  ASSERT (v.pos == 1); // vrhfmt reads the first byte to determine eof
  ASSERT (pos > 0);

  pos -= 1; // Scroll back to type header, we'll write over when we write
  *dest_ppos = pos;

  return SUCCESS;
}

static err_t
vfhmi_write_variable_here (
    hm *h,
    var_hash_entry entry, // The entry to write
    const page *leaf,     // Starting leaf page
    p_size starting_pos,  // Starting position on that page
    error *e)
{
  /**
   * Create a deserializer to scan the variables
   * first to find duplicates or the end (very similar
   * to the get routine)
   */
  p_size pos = starting_pos;
  vwrite_hash_fmt vw = vwhfmt_create (entry);

  while (!vw.done)
    {
      ASSERT (pos <= HL_DATA_LEN);

      /**
       * If we're at the end, grab a new page and reset
       */
      if (pos == HL_DATA_LEN)
        {
          pgno next = hl_get_next (&leaf->hl);

          // No next, need to create a new page
          if (next == 0)
            {
              const page *nextp = pgr_new (h->pager, PG_HASH_LEAF, e);
              if (nextp == NULL)
                {
                  return err_t_from (e);
                }

              page *leafw = pgr_make_writable (h->pager, leaf);
              hl_set_next (&leafw->hl, nextp->pg);

              leaf = nextp;
              pos = 0;
            }
          else
            {
              leaf = pgr_get (PG_HASH_LEAF, next, h->pager, e);
              if (leaf == NULL)
                {
                  return err_t_from (e);
                }
            }

          pos = 0;
        }

      p_size nbytes = HL_DATA_LEN - pos;
      err_t_wrap (vwhfmt_write_out (
                      leaf->hl.data + pos, &nbytes, &vw, e),
                  e);

      pos += nbytes;
    }

  return SUCCESS;
}

err_t
hm_insert (
    hm *h,
    const variable var,
    error *e)
{
  var_hash_entry entry;

  u8 data[2048];
  lalloc alloc = lalloc_create (data, sizeof (data));

  // First, Convert the variable into a var hash entry
  err_t_wrap (var_hash_entry_create (&entry, &var, &alloc, e), e);

  // Then create or get the starting hash leaf page for this key
  const page *leaf = hm_fetch_starting_leaf (h, var.vname, e);
  if (leaf == NULL)
    {
      return err_t_from (e);
    }

  // Scroll to the end of the variable entries
  p_size pos; // The local position within the page
  err_t_wrap (vfhmi_scroll_to_eof (leaf, &pos, h->pager, var.vname, e), e);

  // Write the variable to the end
  err_t_wrap (vfhmi_write_variable_here (h, entry, leaf, pos, e), e);

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
