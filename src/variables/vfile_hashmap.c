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
       * on an already existing file
       */
      return ERR_INVALID_STATE;
    }
  ASSERT (hp.pg == 0);

  return SUCCESS;
}

#define MIN(a, b) ((a) < (b) ? (a) : (b))

err_t
vfhm_get (const vfile_hashmap *h, vmeta *dest, const string key)
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
      return ERR_DOESNT_EXIST;
    }

  /**
   * Fetch the first page
   */
  ret = pgr_get_expect (&leaf, PG_HASH_LEAF, lpg, h->pager);
  if (ret)
    {
      return ret;
    }

  /**
   * Create the deserializer
   */
  vread_hash_fmt v = vrhfmt_create (h->alloc);
  p_size pos = 0; // The position in the hash list
  p_size dlen = hl_data_len (leaf.hl.rlen);

  /**
   * TODO - what if you have a cycle in the linked list?
   * (it would be invalid)
   */
  while (true)
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
              return ERR_DOESNT_EXIST;
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

      p_size nbytes = dlen - pos;
      ret = vrhfmt_read_in (leaf.hl.data + pos, &nbytes, &v);
      if (ret)
        {
          return ret;
        }
      pos += nbytes;

      if (vrhfmt_done (&v))
        {
          string vstr = (string){
            .data = v.vstr,
            .len = v.vstrlen,
          };
          if (string_equal (vstr, key))
            {
              i_log_debug ("Found variable: %.*s\n",
                           vstr.len, vstr.data);
              dest->pgn0 = v.pg0;
              vrhfmt_free_and_reset (&v);
              return SUCCESS;
            }

          vrhfmt_free_and_reset (&v);
        }
    }
}

err_t
vfhm_insert (vfile_hashmap *h, const string key, vmeta value)
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
       * Create the first page
       */
      ret = pgr_new (&leaf, h->pager, PG_HASH_LEAF);
      if (ret)
        {
          return ret;
        }

      /**
       * Save the starting node for the hash page
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
       * Fetch the first page
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

  /**
   * Create a deserializer to scan the variables
   */
  vread_hash_fmt v = vrhfmt_create (h->alloc);
  p_size pos = 0; // The position in the hash list
  p_size dlen = hl_data_len (leaf.hl.rlen);

  /**
   * TODO - what if you have a cycle in the linked list?
   * (it would be invalid)
   */
  while (true)
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
              goto insert_phase;
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

      p_size nbytes = dlen - pos;
      ret = vrhfmt_read_in (leaf.hl.data + pos, &nbytes, &v);
      if (ret)
        {
          return ret;
        }
      pos += nbytes;

      if (vrhfmt_done (&v))
        {
          string vstr = (string){
            .data = v.vstr,
            .len = v.vstrlen,
          };
          if (string_equal (vstr, key))
            {
              i_log_debug ("Found variable: %.*s\n",
                           vstr.len, vstr.data);
              vrhfmt_free_and_reset (&v);
              return ERR_ALREADY_EXISTS;
            }

          vrhfmt_free_and_reset (&v);
        }
    }

insert_phase:
  {

    // we're now aligned on the eof are we on the start or the end?
    vwrite_hash_fmt vw = vwhfmt_create ((vwhfmt_params){
        .vstr = key,
        .tstr = (string){ .data = "TODO", .len = 4 },
        .pg0 = value.pgn0,
    });

    while (true)
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
                goto insert_phase;
              }

            /**
             * Save this current page
             */
            ret = pgr_commit (h->pager, leaf.hl.data, leaf.pg);
            if (ret)
              {
                return ret;
              }

            /**
             * Get the next page
             */
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

        p_size nbytes = dlen - pos;
        vwhfmt_write_out (leaf.hl.data + pos, &nbytes, &vw);
        pos += nbytes;

        if (vwhfmt_done (&vw))
          {
            return SUCCESS;
          }
      }
  }
  ASSERT (0);
}
