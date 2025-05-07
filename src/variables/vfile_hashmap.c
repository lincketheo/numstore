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

  u8 _header[HLRC_PFX_LEN]; // A buffer to read in the header
  p_size hidx = 0;          // Position in the current header
  hl_header header = { 0 }; // The actual current header
  string vstr = { 0 };      // The current variable string
  string tstr = { 0 };      // The current type string
  p_size pos = 0;           // position inside the current node
  u32 vidx = 0;             // Variable id in the list
  p_size dlen = hl_data_len (leaf.in.rlen);

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
              /**
               * This happens when the last header overlaps
               * and we have an invalid next node
               */
              if (hidx < HLRC_PFX_LEN)
                {
                  return ERR_INVALID_STATE;
                }

              /**
               * Otherwise, we expect to be at the EOF header
               */
              if (header.type != HLRCH_EOF)
                {
                  return ERR_INVALID_STATE;
                }

              return ERR_DOESNT_EXIST;
            }
          ret = pgr_get_expect (&leaf, PG_HASH_LEAF, next, h->pager);
          if (ret)
            {
              return ret;
            }
          pos = 0;
        }

      /**
       * First, read into _header
       */
      if (hidx < HLRC_PFX_LEN)
        {
          p_size next = MIN (HLRC_PFX_LEN - hidx, dlen - pos);
          ASSERT (next > 0);

          i_memcpy (_header + hidx, &leaf.hl.data[pos], next);
          hidx += next;
          pos += next;

          /**
           * Continue the loop, we aren't ready to move on
           * Next loop will fetch a new page and (hopefully finish
           * because a header should be less than dlen - although this
           * logic doesn't assume that
           */
          if (hidx != HLRC_PFX_LEN)
            {
              continue;
            }

          /**
           * Otherwise, parse header and move on
           */
          header = hlh_parse (_header);

          switch (header.type)
            {
            case HLRCH_EOF:
              {
                return ERR_DOESNT_EXIST;
              }
            case HLRCH_PRESENT:
              {
                /**
                 * Allocate room for the strings
                 */
                vstr.len = 0;
                vstr.data = lmalloc (h->alloc, header.vstrlen);
                if (vstr.data == NULL)
                  {
                    return ERR_NOMEM;
                  }

                tstr.len = 0;
                tstr.data = lmalloc (h->alloc, header.tstrlen);
                if (tstr.data == NULL)
                  {
                    lfree (h->alloc, vstr.data);
                    return ERR_NOMEM;
                  }
                break;
              }
            case HLRCH_UNKNOWN:
              {
                return ERR_INVALID_STATE;
              }
            case HLRCH_TOMBSTONE:
              {
                /**
                 * Set them to null - later logic will handle this
                 */
                vstr.len = 0;
                tstr.len = 0;
                vstr.data = NULL;
                tstr.data = NULL;
                break;
              }
            }
        }

      ASSERT (hidx == HLRC_PFX_LEN);

      /**
       * Next, read into vstring
       */
      ASSERT (vstr.len <= header.vstrlen);
      if (vstr.len < header.vstrlen)
        {
          p_size next = MIN (header.vstrlen - vstr.len, dlen - pos);
          if (vstr.data && next > 0)
            {
              i_memcpy (vstr.data + vstr.len, &leaf.hl.data[pos], next);
            }
          vstr.len += next;
          pos += next;
        }

      /**
       * Next, read into tstring
       */
      ASSERT (tstr.len <= header.tstrlen);
      if (tstr.len < header.tstrlen && vstr.len == header.vstrlen)
        {
          p_size next = MIN (header.tstrlen - tstr.len, dlen - pos);
          if (tstr.data && next > 0)
            {
              i_memcpy (tstr.data + tstr.len, &leaf.hl.data[pos], next);
            }
          tstr.len += next;
          pos += next;
        }

      if (tstr.len == header.tstrlen && vstr.len == header.vstrlen)
        {
          if (vstr.data && tstr.data && string_equal (vstr, key))
            {
              i_log_debug ("Found variable: %.*s at index: %d\n", vstr.len, vstr.data, vidx);
              dest->pgn0 = header.pg0;
              return SUCCESS;
            }
          else
            {
              vidx += 1;
              hidx = 0;
              if (tstr.data)
                {
                  lfree (h->alloc, tstr.data);
                  tstr = (string){ 0 };
                }
              if (vstr.data)
                {
                  lfree (h->alloc, vstr.data);
                  vstr = (string){ 0 };
                }
            }
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
      ret = pgr_get_expect (&leaf, PG_HASH_LEAF, lpg, h->pager);
      if (ret)
        {
          return ret;
        }
    }

  u8 _header[HLRC_PFX_LEN]; // A buffer to read in the header
  p_size hidx = 0;          // Position in the current header
  hl_header header = { 0 }; // The actual current header
  string vstr = { 0 };      // The current variable string
  string tstr = { 0 };      // The current type string
  p_size pos = 0;           // position inside the current node
  u32 vidx = 0;             // Variable id in the list
  p_size dlen = hl_data_len (leaf.in.rlen);

  /**
   * TODO - what if you have a cycle in the linked list?
   * (it would be invalid)
   *
   * First - Find the EOF
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
              /**
               * This happens when the last header overlaps
               * and we have an invalid next node
               */
              if (hidx < HLRC_PFX_LEN)
                {
                  return ERR_INVALID_STATE;
                }

              /**
               * Otherwise, we expect to be at the EOF header
               */
              if (header.type != HLRCH_EOF)
                {
                  return ERR_INVALID_STATE;
                }

              /**
               * Edge condition - fits perfectly
               *
               * TODO - swap out header
               */
              ret = pgr_new (&leaf, h->pager, PG_HASH_LEAF);
              if (ret)
                {
                  return ret;
                }
              goto insert_phase;
            }
          ret = pgr_get_expect (&leaf, PG_HASH_LEAF, next, h->pager);
          if (ret)
            {
              return ret;
            }
          pos = 0;
        }

      /**
       * First, read into _header
       */
      if (hidx < HLRC_PFX_LEN)
        {
          p_size next = MIN (HLRC_PFX_LEN - hidx, dlen - pos);
          ASSERT (next > 0);

          i_memcpy (_header + hidx, &leaf.hl.data[pos], next);
          hidx += next;
          pos += next;

          /**
           * Continue the loop, we aren't ready to move on
           * Next loop will fetch a new page and (hopefully finish
           * because a header should be less than dlen - although this
           * logic doesn't assume that
           */
          if (hidx != HLRC_PFX_LEN)
            {
              continue;
            }

          /**
           * Otherwise, parse header and move on
           */
          header = hlh_parse (_header);

          switch (header.type)
            {
            case HLRCH_EOF:
              {
                /**
                 * TODO - swap out header
                 */
                goto insert_phase;
              }
            case HLRCH_PRESENT:
              {
                /**
                 * Allocate room for the strings
                 */
                vstr.len = 0;
                vstr.data = lmalloc (h->alloc, header.vstrlen);
                if (vstr.data == NULL)
                  {
                    return ERR_NOMEM;
                  }

                tstr.len = 0;
                tstr.data = lmalloc (h->alloc, header.tstrlen);
                if (tstr.data == NULL)
                  {
                    lfree (h->alloc, vstr.data);
                    return ERR_NOMEM;
                  }
                break;
              }
            case HLRCH_UNKNOWN:
              {
                return ERR_INVALID_STATE;
              }
            case HLRCH_TOMBSTONE:
              {
                /**
                 * Set them to null - later logic will handle this
                 */
                vstr.len = 0;
                tstr.len = 0;
                vstr.data = NULL;
                tstr.data = NULL;
                break;
              }
            }
        }

      ASSERT (hidx == HLRC_PFX_LEN);

      /**
       * Next, read into vstring
       */
      ASSERT (vstr.len <= header.vstrlen);
      if (vstr.len < header.vstrlen)
        {
          p_size next = MIN (header.vstrlen - vstr.len, dlen - pos);
          if (vstr.data && next > 0)
            {
              i_memcpy (vstr.data + vstr.len, &leaf.hl.data[pos], next);
            }
          vstr.len += next;
          pos += next;
        }

      /**
       * Next, read into tstring
       */
      ASSERT (tstr.len <= header.tstrlen);
      if (tstr.len < header.tstrlen && vstr.len == header.vstrlen)
        {
          p_size next = MIN (header.tstrlen - tstr.len, dlen - pos);
          if (tstr.data && next > 0)
            {
              i_memcpy (tstr.data + tstr.len, &leaf.hl.data[pos], next);
            }
          tstr.len += next;
          pos += next;
        }

      if (tstr.len == header.tstrlen && vstr.len == header.vstrlen)
        {
          if (vstr.data && tstr.data && string_equal (vstr, key))
            {
              i_log_debug ("Found variable: %.*s at index: %d\n", vstr.len, vstr.data, vidx);
              return ERR_ALREADY_EXISTS;
            }
          else
            {
              vidx += 1;
              hidx = 0;
              if (tstr.data)
                {
                  lfree (h->alloc, tstr.data);
                  tstr = (string){ 0 };
                }
              if (vstr.data)
                {
                  lfree (h->alloc, vstr.data);
                  vstr = (string){ 0 };
                }
            }
        }
    }

insert_phase:

  return SUCCESS;
}
