#include "variables/vfile_hashmap.h"

#include "dev/assert.h"
#include "ds/strings.h"
#include "errors/error.h"
#include "intf/logging.h"
#include "intf/stdlib.h"
#include "mm/lalloc.h"
#include "paging/file_pager.h"
#include "paging/page.h"
#include "paging/pager.h"
#include "paging/types/hash_leaf.h"
#include "paging/types/hash_page.h"
#include "variables/variable.h"
#include "variables/vhash_fmt.h"

DEFINE_DBG_ASSERT_I (vfile_hashmap, vfile_hashmap, v)
{
  ASSERT (v);
  ASSERT (v->pager);
}

#define MAX_VSTR 512
#define MAX_TSTR 512

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

  page hp;
  err_t_wrap (pgr_new (&hp, h->pager, PG_HASH_PAGE, e), e);

  if (hp.pg != 0)
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

static err_t
vfhmg_fetch_starting_leaf (
    page *leaf,       // Destination for the leaf
    pager *p,         // For fetching pages
    const string key, // The key we're fetching the leaf for
    error *e          // Any errors along the way
)
{
  page cur;    // Current page we're looking at
  p_size hpos; // Hash index in the hash page
  pgno lpg;    // Leaf page number

  /**
   * Fetch the first hash page
   */
  err_t_wrap (pgr_get_expect (&cur, PG_HASH_PAGE, 0, p, e), e);

  /**
   * Hash the key
   */
  hpos = hp_get_hash_pos (&cur.hp, key);
  lpg = hp_get_pgno (&cur.hp, hpos);

  /**
   * Hash's of 0 mean the hash position hasn't
   * been indexed before (0 being a magic number)
   */
  if (lpg == 0)
    {
      return ERR_DOESNT_EXIST;
    }

  /**
   * Fetch the first leaf page
   */
  err_t_wrap (pgr_get_expect (&cur, PG_HASH_LEAF, lpg, p, e), e);

  *leaf = cur;
  return SUCCESS;
}

static err_t
vfhmg_scroll_leafs (
    vread_hash_fmt *result, // Save the output here
    pager *p,               // To fetch pages
    page cur,               // Starting page
    const string key,       // Key we're looking for
    lalloc *alloc,          // Allocator for resulting variable
    error *e                // Any errors along the way
)
{
  /**
   * Create the deserializer
   */
  vread_hash_fmt v = vrhfmt_create (alloc);
  p_size pos = 0; // The byte position in the current hash list
  p_size dlen = hl_data_len (cur.hl.rlen);

  /**
   * TODO - what if you have a cycle in the linked list?
   * (it would be invalid)
   */
  while (true)
    {
      ASSERT (pos <= dlen);

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
            return ERR_FALLBACK;
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
                    *result = v;
                    return SUCCESS;
                  }
                vrhfmt_reset (&v);
              }
            break;
          }
        default:
          {
            /**
             * We need to fetch a new page
             */
            if (pos == dlen)
              {
                pgno next = hl_get_next (&cur.hl);
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

                err_t_wrap (pgr_get_expect (&cur, PG_HASH_LEAF, next, p, e), e);

                pos = 0;
              }

            /**
             * Do the actual read
             */
            p_size nbytes = dlen - pos;
            err_t_wrap (vrhfmt_read_in (cur.hl.data + pos, &nbytes, &v, e), e);
            pos += nbytes;

            ASSERT (nbytes > 0); // Avoid infinite loops
          }
        }
    }
}

err_t
vfhm_get (
    const vfile_hashmap *h,
    vmeta *dest,
    const string key,
    lalloc *alloc,
    error *e)
{
  /**
   * Center yourself on the starting leaf node
   */
  page cur;
  err_t_wrap (vfhmg_fetch_starting_leaf (&cur, h->pager, key, e), e);

  /**
   * Scroll leafs until you find yours
   */
  vread_hash_fmt entry;
  u8 search[MAX_VSTR + MAX_TSTR];
  lalloc searcha = lalloc_create (search, sizeof (search));
  err_t_wrap (vfhmg_scroll_leafs (&entry, h->pager, cur, key, &searcha, e), e);

  ASSERT (entry.state == VHFMT_DONE);

  /**
   * Convert the reader to a var_hash_entry
   */
  var_hash_entry result = (var_hash_entry){
    .vstr = entry.vstr,
    .vlen = entry.vstrlen,
    .tstr = entry.tstr,
    .tlen = entry.tstrlen,
    .pg0 = entry.pg0,
  };

  /**
   * Convert to a variable meta
   */
  err_t ret = vhe_to_vm (dest, &result, alloc, e);
  if (ret == ERR_TYPE_DESER)
    {
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
static err_t
vfhmi_fetch_starting_leaf (
    page *hl,
    vfile_hashmap *h,
    const string key,
    error *e)
{
  vfile_hashmap_assert (h);

  page hp;     // First Hash page
  page leaf;   // Leaf for traversing
  p_size hpos; // Hash index
  pgno lpg;    // Leaf page

  /**
   * Fetch the first hash page
   */
  err_t_wrap (pgr_get_expect (&hp, PG_HASH_PAGE, 0, h->pager, e), e);

  /**
   * Hash the key
   */
  hpos = hp_get_hash_pos (&hp.hp, key);
  lpg = hp_get_pgno (&hp.hp, hpos);

  if (lpg == 0)
    {
      /**
       * Create the first leaf page
       */
      err_t_wrap (pgr_new (&leaf, h->pager, PG_HASH_LEAF, e), e);

      /**
       * Save the starting node of the hash page
       */
      hp_set_hash (&hp.hp, hpos, leaf.pg);
      err_t_wrap (pgr_commit (h->pager, hp.hp.raw, hp.pg, e), e);
    }
  else
    {
      /**
       * Otherwise fetch the first page
       */
      err_t_wrap (pgr_get_expect (&leaf, PG_HASH_LEAF, lpg, h->pager, e), e);
    }

  *hl = leaf;

  return SUCCESS;
}

/**
 * Scrolls to the end of this variable hash leaf page
 */
static err_t
vfhmi_scroll_to_eof (
    page *dest_pg,     // The page we landed on
    p_size *dest_ppos, // The local page pos at the end
    pager *p,          // Used to fetch pages
    const string key,  // Used to check for dupes
    error *e)
{
  ASSERT (dest_pg->type == PG_HASH_LEAF);
  page leaf = *dest_pg;

  u8 working[MAX_VSTR + MAX_TSTR];
  lalloc alloc = lalloc_create (working, sizeof (working));

  /**
   * Create a deserializer to scan the variables
   * first to find duplicates or the end (very similar
   * to the get routine)
   */
  vread_hash_fmt v = vrhfmt_create (&alloc); // The reader
  p_size pos = 0;                            // Local position on page
  p_size dlen = hl_data_len (leaf.hl.rlen);  // Save byte length of leaf

  while (v.state != VHFMT_EOF)
    {
      ASSERT (pos <= dlen);

      switch (v.state)
        {

        case VHFMT_CORRUPT:
          {
            UNREACHABLE ();
            return ERR_FALLBACK;
          }
        case VHFMT_EOF:
          {
            UNREACHABLE ();
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
                vrhfmt_reset (&v);

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
            if (pos == dlen)
              {
                pgno next = hl_get_next (&leaf.hl);
                if (next == 0)
                  {
                    /**
                     * Deserializer handles EOF, not paging
                     * so this is corrupt because there should be
                     * more
                     */
                    vrhfmt_reset (&v);
                    return error_causef (e, ERR_CORRUPT, "Hash Leaf expecting next page");
                  }

                err_t_wrap (pgr_get_expect (&leaf, PG_HASH_LEAF, next, p, e), e);
                pos = 0; // Reset to the start of the page
              }

            /**
             * Read into vrhfmt
             */
            p_size nbytes = dlen - pos;
            err_t_wrap (vrhfmt_read_in (leaf.hl.data + pos, &nbytes, &v, e), e);
            pos += nbytes;

            ASSERT (nbytes > 0); // Avoid infinite loops
          }
        }
    }

  ASSERT (v.state == VHFMT_EOF);
  ASSERT (v.pos == 1); // vrhfmt reads the first byte to determine eof
  ASSERT (pos > 0);

  pos -= 1; // Go back, we'll write over this byte when we write
  *dest_ppos = pos;
  *dest_pg = leaf;

  return SUCCESS;
}

static err_t
vfhmi_write_variable_here (
    pager *p,
    var_hash_entry entry,
    page starting_page,
    p_size starting_pos,
    error *e)
{
  /**
   * Create a deserializer to scan the variables
   * first to find duplicates or the end (very similar
   * to the get routine)
   */
  page leaf = starting_page;
  p_size pos = starting_pos;
  p_size dlen = hl_data_len (leaf.hl.rlen);
  vwrite_hash_fmt vw = vwhfmt_create (entry);

  while (!vw.done)
    {
      ASSERT (pos <= dlen);

      /**
       * If we're at the end, grab a new page and reset
       */
      if (pos == dlen)
        {
          pgno next = hl_get_next (&leaf.hl);
          if (next == 0)
            {
              page nextp;
              err_t_wrap (pgr_new (&nextp, p, PG_HASH_LEAF, e), e);

              hl_set_next (&leaf.hl, nextp.pg);
              err_t_wrap (pgr_commit (p, leaf.hl.raw, leaf.pg, e), e);

              leaf = nextp;
              pos = 0;
            }
          else
            {
              err_t_wrap (pgr_commit (p, leaf.hl.data, leaf.pg, e), e);
              err_t_wrap (pgr_get_expect (&leaf, PG_HASH_LEAF, next, p, e), e);
            }

          pos = 0;
        }
    }

  p_size nbytes = dlen - pos;
  err_t_wrap (vwhfmt_write_out (leaf.hl.data + pos, &nbytes, &vw, e), e);

  pos += nbytes;

  return SUCCESS;
}

err_t
vfhm_insert (
    vfile_hashmap *h,
    const string key,
    vmeta value,
    error *e)
{
  page leaf;
  var_hash_entry entry;

  // Stores the serialized variable from [key, value]
  u8 vspace[MAX_VSTR + MAX_TSTR];
  lalloc myvar = lalloc_create (vspace, sizeof (vspace));

  /**
   * Convert the variable meta info into
   * a var hash entry
   */
  err_t_wrap (vm_to_vhe (&entry, key, value, &myvar, e), e);

  /**
   * Create or get the starting hash leaf
   * page for this key
   */
  err_t_wrap (vfhmi_fetch_starting_leaf (&leaf, h, key, e), e);

  /**
   * Scroll to the end of the variable entries
   */
  p_size pos; // Local page position
  err_t_wrap (vfhmi_scroll_to_eof (&leaf, &pos, h->pager, key, e), e);

  /**
   * Write the variable to the end
   */
  err_t_wrap (vfhmi_write_variable_here (h->pager, entry, leaf, pos, e), e);

  return SUCCESS;
}
