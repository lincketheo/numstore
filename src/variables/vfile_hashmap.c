#include "variables/vfile_hashmap.h"
#include "dev/assert.h"
#include "dev/errors.h"
#include "ds/strings.h"
#include "intf/logging.h"
#include "intf/stdlib.h"
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

vfile_hashmap
vfhm_create (vfhm_params params)
{
  vfile_hashmap ret = {
    .pager = params.pager,
    .alloc = params.alloc,
  };

  vfile_hashmap_assert (&ret);

  return ret;
}

err_t
vfhm_create_hashmap (vfile_hashmap *h)
{
  vfile_hashmap_assert (h);

  err_t ret;

  page hp;
  if ((ret = pgr_new (&hp, h->pager, PG_HASH_PAGE)))
    {
      return ret;
    }
  if (hp.pg != 0)
    {
      /**
       * Might be trying to create a database
       * on an already existing file?
       *
       * I think this could be blocked by an ASSERT
       * because we should have already checked that
       */
      return ERR_INVALID_STATE;
    }

  return SUCCESS;
}

#define MIN(a, b) ((a) < (b) ? (a) : (b))

static err_t
vfhm_get_center_on_page (
    page *leaf,
    const vfile_hashmap *h,
    const string key)
{
  vfile_hashmap_assert (h);

  err_t ret;   // Error code
  page cur;    // Current page we're looking at
  p_size hpos; // Hash index in the hash page
  pgno lpg;    // Leaf page number

  /**
   * Fetch the first hash page
   */
  ret = pgr_get_expect (&cur, PG_HASH_PAGE, 0, h->pager);
  if (ret)
    {
      return ret;
    }

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
  ret = pgr_get_expect (&cur, PG_HASH_LEAF, lpg, h->pager);
  if (ret)
    {
      return ret;
    }

  *leaf = cur;
  return SUCCESS;
}

static err_t
vfhm_get_scroll_leafs (
    vread_hash_fmt *result,
    const vfile_hashmap *h,
    page cur,
    const string key)
{
  err_t ret;

  /**
   * Create the deserializer
   */
  vread_hash_fmt v = vrhfmt_create (h->alloc);
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
             * I don't think you can get here
             * because putting it in an invalid
             * state would be caught by our error handlers
             */
            ASSERT (0);
            return ERR_INVALID_STATE;
          }
        case VHFMT_EOF:
          {
            return ERR_DOESNT_EXIST;
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
                    i_log_debug ("Found variable: %.*s\n",
                                 vstr.len, vstr.data);

                    *result = v;

                    ret = SUCCESS;
                    goto theend;
                  }

                vrhfmt_free_and_reset (&v);
              }
            break;
          }
        default:
          {
            if (pos == dlen)
              {
                pgno next = hl_get_next (&cur.hl);
                if (next == 0)
                  {
                    // Deserializer handles EOF, not paging
                    // so this is corrupt because there should be
                    // more
                    ret = ERR_INVALID_STATE;
                    goto theend;
                  }

                ret = pgr_get_expect (&cur, PG_HASH_LEAF, next, h->pager);
                if (ret)
                  {
                    goto theend;
                  }

                pos = 0;
              }

            p_size nbytes = dlen - pos;
            ret = vrhfmt_read_in (cur.hl.data + pos, &nbytes, &v);
            if (ret)
              {
                goto theend;
              }
            pos += nbytes;
            ASSERT (nbytes > 0); // Avoid infinite loops
          }
        }
    }

theend:
  if (ret)
    {
      vrhfmt_free_and_reset_forgiving (&v);
    }
  return ret;
}

err_t
vfhm_get (const vfile_hashmap *h, vmeta *dest, const string key)
{
  page cur;
  err_t ret = vfhm_get_center_on_page (&cur, h, key);
  if (ret)
    {
      return ret;
    }

  vread_hash_fmt entry;
  ret = vfhm_get_scroll_leafs (&entry, h, cur, key);
  if (ret)
    {
      return ret;
    }
  ASSERT (entry.state == VHFMT_DONE);

  var_hash_entry result = (var_hash_entry){
    .vstr = entry.vstr,
    .vlen = entry.vstrlen,
    .tstr = entry.tstr,
    .tlen = entry.tstrlen,
    .pg0 = entry.pg0,
  };

  ret = vhe_to_vm (dest, &result, h->alloc);
  vrhfmt_free_and_reset (&entry);
  return ret;
}

static err_t
vfhm_insert_center_on_page (page *hl, vfile_hashmap *h, const string key)
{
  vfile_hashmap_assert (h);

  err_t ret;   // Error code
  page hp;     // First Hash page
  page leaf;   // Leaf for traversing
  p_size hpos; // Hash index
  pgno lpg;    // Leaf page

  /**
   * Fetch the first hash page
   */
  ret = pgr_get_expect (&hp, PG_HASH_PAGE, 0, h->pager);
  if (ret)
    {
      return ret;
    }

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
      ret = pgr_new (&leaf, h->pager, PG_HASH_LEAF);
      if (ret)
        {
          return ret;
        }

      /**
       * Save the starting node of the hash page
       */
      hp_set_hash (&hp.hp, hpos, leaf.pg);
      ret = pgr_commit (h->pager, hp.hp.raw, hp.pg);
      if (ret)
        {
          return ret;
        }
    }
  else
    {
      /**
       * Otherwise fetch the first page
       */
      ret = pgr_get_expect (
          &leaf,
          PG_HASH_LEAF,
          lpg, h->pager);
      if (ret)
        {
          return ret;
        }
    }

  *hl = leaf;

  return SUCCESS;
}

static err_t
vfhm_insert_scroll_to_eof (
    page *dest_page,
    p_size *dest_page_position,
    vfile_hashmap *h,
    const string key)
{
  err_t ret = SUCCESS;
  page leaf = *dest_page;

  /**
   * Create a deserializer to scan the variables
   * first to find duplicates or the end (very similar
   * to the get routine)
   */
  vread_hash_fmt v = vrhfmt_create (h->alloc);
  p_size pos = 0; // The byte position in the hash list
  p_size dlen = hl_data_len (leaf.hl.rlen);

  while (v.state != VHFMT_EOF)
    {
      ASSERT (pos <= dlen);

      switch (v.state)
        {

        case VHFMT_CORRUPT:
          {
            ASSERT (0);
            return ERR_INVALID_STATE;
          }
        case VHFMT_EOF:
          {
            ASSERT (0);
            break;
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
                    i_log_debug ("Found variable: %.*s\n",
                                 vstr.len, vstr.data);
                    ret = ERR_ALREADY_EXISTS;
                    goto theend;
                  }
                vrhfmt_free_and_reset (&v);
              }
            break;
          }
        default:
          {
            if (pos == dlen)
              {
                pgno next = hl_get_next (&leaf.hl);
                if (next == 0)
                  {
                    ret = ERR_INVALID_STATE;
                    goto theend;
                  }

                ret = pgr_get_expect (&leaf, PG_HASH_LEAF, next, h->pager);
                if (ret)
                  {
                    goto theend;
                  }

                pos = 0;
              }

            p_size nbytes = dlen - pos;
            ret = vrhfmt_read_in (leaf.hl.data + pos, &nbytes, &v);
            if (ret)
              {
                goto theend;
              }
            pos += nbytes;
            ASSERT (nbytes > 0); // Avoid infinite loops
          }
        }
    }

  ASSERT (v.state == VHFMT_EOF);
  ASSERT (v.pos == 1);
  ASSERT (pos > 0);

  pos -= 1; // We would have just read the eof byte
  *dest_page_position = pos;
  *dest_page = leaf;

theend:
  vrhfmt_free_and_reset_forgiving (&v);
  return ret;
}

static err_t
vfhm_insert_write_variable_here (
    vfile_hashmap *h,
    var_hash_entry entry,
    page starting_page,
    p_size starting_pos)
{
  err_t ret;

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
              ret = pgr_new (&nextp, h->pager, PG_HASH_LEAF);
              if (ret)
                {
                  return ret;
                }
              hl_set_next (&leaf.hl, nextp.pg);
              ret = pgr_commit (h->pager, leaf.hl.raw, leaf.pg);
              if (ret)
                {
                  return ret;
                }
              leaf = nextp;
              pos = 0;
            }
          else
            {
              ret = pgr_commit (h->pager, leaf.hl.data, leaf.pg);
              if (ret)
                {
                  return ret;
                }

              ret = pgr_get_expect (
                  &leaf,
                  PG_HASH_LEAF,
                  next, h->pager);

              if (ret)
                {
                  return ret;
                }

              pos = 0;
            }
        }

      p_size nbytes = dlen - pos;
      ret = vwhfmt_write_out (leaf.hl.data + pos, &nbytes, &vw);
      if (ret)
        {
          return ret;
        }
      pos += nbytes;
    }

  return SUCCESS;
}

err_t
vfhm_insert (vfile_hashmap *h, const string key, vmeta value)
{
  page leaf;
  err_t ret;
  var_hash_entry entry;

  ret = vm_to_vhe (&entry, key, value, h->alloc);
  if (ret)
    {
      // Don't free it
      return ret;
    }

  ret = vfhm_insert_center_on_page (&leaf, h, key);
  if (ret)
    {
      goto theend;
    }

  p_size pos;
  ret = vfhm_insert_scroll_to_eof (&leaf, &pos, h, key);
  if (ret)
    {
      goto theend;
    }

  ret = vfhm_insert_write_variable_here (h, entry, leaf, pos);
  if (ret)
    {
      goto theend;
    }

theend:
  var_hash_entry_free (&entry, h->alloc);
  return ret;
}
