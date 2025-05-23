#include "variables/vfile_hashmap.h"

#include "errors/error.h"
#include "mm/lalloc.h"
#include "paging/page.h"
#include "paging/pager.h"
#include "variables/vhash_fmt.h" // vread_hash_fmt

DEFINE_DBG_ASSERT_I (vfile_hashmap, vfile_hashmap, v)
{
  ASSERT (v);
  ASSERT (v->pager);
}

vfile_hashmap
vfhm_create (pager *pager)
{
  vfile_hashmap ret = (vfile_hashmap){
    .pager = pager,
  };

  vfile_hashmap_assert (&ret);

  return ret;
}

err_t
vfhm_create_hashmap (vfile_hashmap *h, error *e)
{
  vfile_hashmap_assert (h);

  page *root = pgr_new (h->pager, PG_HASH_PAGE, e);

  if (root->pg != 0)
    {
      /**
       * Might be trying to create a database
       * on an already existing file?
       *
       * I think this could be blocked by an ASSERT
       * because we should have already checked that
       */
      return error_causef (
          e, ERR_CORRUPT,
          "VFile Hash Map: "
          "Hash page already exists");
    }

  return SUCCESS;
}

#define MIN(a, b) ((a) < (b) ? (a) : (b))

/**
 * Finds the starting leaf for the variable
 * with key name [key]
 * Errors:
 *    - ERR_IO / ERR_CORRUPT (pgr_get_expect) - e.g. leaf page
 *      says it exists on page 0 but doesn't or page 0 doesn't
 *      exist
 *    - ERR_DOESNT_EXIST - Leaf doesn't exist
 */
static page *
vfhmg_fetch_starting_leaf (
    pager *p,         // For fetching pages
    const string key, // The key we're fetching the leaf for
    error *e          // Any errors along the way
)
{
  page *cur;   // Current page we're looking at
  p_size hpos; // Hash index in the hash page
  pgno lpg;    // Leaf page number

  /**
   * Fetch the first hash page
   */
  cur = pgr_get_expect_rw (PG_HASH_PAGE, 0, p, e);
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
  return pgr_get_expect_rw (PG_HASH_LEAF, lpg, p, e);
}

static err_t
vfhmg_scroll_leafs (
    vread_hash_fmt *dest, // result
    pager *p,             // To fetch pages
    page *start,          // Starting page
    const string key,     // Key we're looking for
    error *e              // Any errors along the way
)
{
  /**
   * Create the deserializer
   */
  vread_hash_fmt v = vrhfmt_create ();
  p_size pos = 0; // The byte position in the current hash list
  page *cur = start;

  /**
   * TODO - what if you have a cycle in the linked list?
   * (it would be invalid)
   */
  while (true)
    {
      ASSERT (pos <= HL_DATA_LEN);

      switch (v.state)
        {

        case VHFMT_CORRUPT:
          {
            /**
             * You can's get here
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
            if (!v.is_tombstone)
              {
                string vstr = (string){
                  .data = v.vstr,
                  .len = v.vstrlen,
                };
                if (string_equal (vstr, key))
                  {
                    *dest = v;
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

                cur = pgr_get_expect_rw (PG_HASH_LEAF, next, p, e);
                if (cur == NULL)
                  {
                    return err_t_from (e);
                  }

                pos = 0;
              }

            /**
             * Do the actual read
             */
            p_size nbytes = HL_DATA_LEN - pos;
            err_t_wrap (vrhfmt_read_in (
                            cur->hl.data + pos,
                            &nbytes, &v, e),
                        e);
            pos += nbytes;

            ASSERT (nbytes > 0); // Avoid infinite loops
          }
        }
    }
}

err_t
vfhm_get (
    const vfile_hashmap *h,
    variable *dest,
    const string key,
    lalloc *alloc,
    error *e)
{
  /**
   * Center yourself on the starting leaf node
   */
  page *cur = vfhmg_fetch_starting_leaf (h->pager, key, e);
  if (cur == NULL)
    {
      return err_t_from (e);
    }

  /**
   * Scroll leafs until you find yours
   */
  vread_hash_fmt entry;
  err_t_wrap (vfhmg_scroll_leafs (&entry, h->pager, cur, key, e), e);

  ASSERT (entry.state == VHFMT_DONE);

  /**
   * Convert the reader to a var_hash_entry
   */
  var_hash_entry result = vrhfmt_consume (&entry);

  /**
   * Convert to a variable meta
   */
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
 */
static page *
vfhmi_fetch_starting_leaf (
    vfile_hashmap *h,
    const string key,
    error *e)
{
  vfile_hashmap_assert (h);

  page *hp;    // First Hash page
  page *leaf;  // Leaf for traversing
  p_size hpos; // Hash index
  pgno lpg;    // Leaf page

  /**
   * Fetch the first hash page
   */
  hp = pgr_get_expect_rw (PG_HASH_PAGE, 0, h->pager, e);
  if (hp == NULL)
    {
      return NULL;
    }

  /**
   * Hash the key
   */
  hpos = hp_get_hash_pos (&hp->hp, key);
  lpg = hp_get_pgno (&hp->hp, hpos);

  if (lpg == 0)
    {
      /**
       * Create the first leaf page
       */
      leaf = pgr_new (h->pager, PG_HASH_LEAF, e);

      /**
       * Save the starting node of the hash page
       */
      hp_set_hash (&hp->hp, hpos, leaf->pg);
      if (pgr_write (h->pager, hp, e), e)
        {
          return NULL;
        }
    }
  else
    {
      /**
       * Otherwise fetch the first page
       */
      leaf = pgr_get_expect_rw (PG_HASH_LEAF, lpg, h->pager, e);
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
    page *leaf,        // The starting leaf
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

                leaf = pgr_get_expect_rw (PG_HASH_LEAF, next, p, e);
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
    pager *p,
    var_hash_entry entry, // The entry to write
    page *leaf,           // Starting leaf page
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
          if (next == 0)
            {
              page *nextp = pgr_new (p, PG_HASH_LEAF, e);
              if (nextp == NULL)
                {
                  return err_t_from (e);
                }

              hl_set_next (&leaf->hl, nextp->pg);
              err_t_wrap (pgr_write (p, leaf, e), e);

              leaf = nextp;
              pos = 0;
            }
          else
            {
              err_t_wrap (pgr_write (p, leaf, e), e);
              leaf = pgr_get_expect_rw (PG_HASH_LEAF, next, p, e);
              if (leaf == NULL)
                {
                  return err_t_from (e);
                }
            }

          pos = 0;
        }

      p_size nbytes = HL_DATA_LEN - pos;
      err_t_wrap (vwhfmt_write_out (leaf->hl.data + pos, &nbytes, &vw, e), e);

      pos += nbytes;
    }

  return SUCCESS;
}

err_t
vfhm_insert (
    vfile_hashmap *h,
    const variable var,
    lalloc *alloc,
    error *e)
{
  var_hash_entry entry;

  u32 alloc_state = lalloc_get_state (alloc); // to reset at the end

  /**
   * First, Convert the variable into a var hash entry
   */
  if (var_hash_entry_create (&entry, &var, alloc, e))
    {
      goto theend;
    }

  /**
   * Then create or get the starting hash leaf page for this key
   */
  page *leaf = vfhmi_fetch_starting_leaf (h, var.vname, e);
  if (leaf == NULL)
    {
      goto theend;
    }

  /**
   * Scroll to the end of the variable entries
   */
  p_size pos;
  if (vfhmi_scroll_to_eof (leaf, &pos, h->pager, var.vname, e))
    {
      goto theend;
    }

  /**
   * Write the variable to the end
   */
  if (vfhmi_write_variable_here (h->pager, entry, leaf, pos, e))
    {
      goto theend;
    }

theend:
  lalloc_reset_to_state (alloc, alloc_state);
  return err_t_from (e);
}
