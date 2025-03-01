#include "paging.h"
#include "config.h"
#include "dev/assert.h"
#include "dev/errors.h"
#include "intf/io.h"
#include "intf/logging.h"
#include "intf/stdlib.h"
#include "sds.h"
#include "types.h"

///////////////////////////// FILE PAGING

DEFINE_DBG_ASSERT_I (file_pager, file_pager, p)
{
  ASSERT (p);
  i_file_assert (p->f);
}

static inline i64
fpgr_len (file_pager *p)
{
  // Get the file size
  // TODO:OPT - cache the file size and increment it
  // - eliminate calls here - delegate to truncate function
  i64 size = i_file_size (p->f);
  if (size < 0)
    {
      i_log_warn ("Error encountered while trying to fetch "
                  "last page size in file_pager\n");
      return size;
    }

  // TODO:OPT if cahce from above, this wouldn't need to be
  // here because program correctness is file size source of truth
  // in memory
  if (size % app_config.page_size != 0)
    {
      i_log_error ("The database is in an invalid state "
                   "when trying to create a new page. File size "
                   "is not a multiple of page size: %" PRIu32 " bytes\n",
                   app_config.page_size);
      return ERR_INVALID_STATE;
    }

  return size;
}

int
fpgr_new (file_pager *p, u64 *dest)
{
  file_pager_assert (p);
  ASSERT (dest);

  i64 size = fpgr_len (p);

  u64 newsize = (u64)size + app_config.page_size;
  int ret = i_truncate (p->f, newsize);
  if (ret)
    {
      return ret;
    }

  *dest = size / app_config.page_size;

  return SUCCESS;
}

int
fpgr_delete (file_pager *p, u64 ptr __attribute__ ((unused)))
{
  // TODO
  file_pager_assert (p);
  ASSERT (ptr > 0);
  return SUCCESS;
}

int
fpgr_get (file_pager *p, u8 *dest, u64 ptr)
{
  file_pager_assert (p);
  ASSERT (dest);

  // Read all from file
  i64 nread = i_read_all (
      p->f, dest,
      app_config.page_size,
      ptr * app_config.page_size);

  if (nread == 0)
    {
      i_log_error ("Early EOF encountered on fpgr_get\n");
      return ERR_INVALID_STATE;
    }

  if (nread < 0)
    {
      i_log_warn ("Invalid read encountered on fpgr_get\n");
      return ERR_IO;
    }

  return SUCCESS;
}

int
fpgr_get_or_create (file_pager *p, u8 *dest, u64 *ptr)
{
  file_pager_assert (p);
  ASSERT (dest);
  ASSERT (ptr);

  file_pager_assert (p);
  ASSERT (dest);

  // Get the file size
  // TODO:OPT - See TODO:OPT at the top of this section

  i64 nread = i_read_all (
      p->f, dest,
      app_config.page_size,
      *ptr * app_config.page_size);

  if (nread == 0)
    {
      int ret = fpgr_new (p, ptr);
      if (ret)
        {
          return ret;
        }
      return SUCCESS;
    }

  if (nread < 0)
    {
      i_log_warn ("Invalid read encountered on fpgr_get\n");
      return ERR_IO;
    }

  return SUCCESS;
}

int
fpgr_commit (file_pager *p, const u8 *src, u64 ptr)
{
  file_pager_assert (p);
  ASSERT (src);

  int ret = i_write_all (
      p->f, src,
      app_config.page_size,
      ptr * app_config.page_size);
  if (ret)
    {
      return ret;
    }

  return SUCCESS;
}

///////////////////////////// MEMORY PAGE

DEFINE_DBG_ASSERT_I (memory_page, memory_page, p)
{
  ASSERT (p);
}

void
mp_create (memory_page *dest, u64 clock, u64 ptr)
{
  i_memset (&dest->page, 0, app_config.page_size);
  dest->laccess = clock;
  dest->ptr = ptr;
}

void
mp_access (memory_page *m, u64 now)
{
  m->laccess = now;
}

u64
mp_check (const memory_page *m, u64 now)
{
  memory_page_assert (m);
  ASSERT (now > m->laccess);
  return now - m->laccess;
}

///////////////////////////// MEMORY PAGER

DEFINE_DBG_ASSERT_I (memory_pager, memory_pager, p)
{
  ASSERT (p);

  // Memory wierdness but this is in an assert
  ASCOPE (u64 used_pages[app_config.mpgr_len]);

  // Ensure no duplicates
  for (u32 i = 0; i < app_config.mpgr_len; ++i)
    {
      if (!p->is_present[i])
        {
          continue;
        }

      memory_page_assert (&p->pages[i]);

      // Check that this page hasn't been used before
      ASCOPE (u64 next = p->pages[i].ptr);
      for (u32 j = 0; j < i; ++j)
        {
          if (p->is_present[j])
            {
              ASSERT (next != used_pages[j]);
            }
        }
      ASCOPE (used_pages[i] = next);
    }
}

int
mpgr_find_avail (const memory_pager *p)
{
  memory_pager_assert (p);

  // TODO - OPTIMIZATION - This could be a hash map
  for (u32 i = 0; i < app_config.mpgr_len; ++i)
    {
      if (!p->is_present[i])
        {
          return i;
        }
    }
  return -1;
}

u8 *
mpgr_new (memory_pager *p, u64 ptr)
{
  memory_pager_assert (p);

  int i = mpgr_find_avail (p);
  ASSERT (i >= 0);

  mp_create (&p->pages[i], p->clock++, ptr);
  p->is_present[i] = 1;

  return p->pages[i].page;
}

u8 *
mpgr_get_by_ptr (memory_pager *p, u64 ptr)
{
  memory_pager_assert (p);
  int idx = mpgr_check_page_exists (p, ptr);

  ASSERT (idx >= 0);
  ASSERT ((u32)idx < app_config.mpgr_len);
  ASSERT (p->is_present[idx]);

  mp_access (&p->pages[idx], p->clock++);

  return p->pages[idx].page;
}

// Retrieves page ptr page must exist
u8 *
mpgr_get_by_idx (memory_pager *p, u32 idx)
{
  memory_pager_assert (p);

  ASSERT (idx < app_config.mpgr_len);
  ASSERT (p->is_present[idx]);

  mp_access (&p->pages[idx], p->clock++);

  return p->pages[idx].page;
}

int
mpgr_check_page_exists (const memory_pager *p, u64 ptr)
{
  // TODO:OPT - use mini hash table
  for (u32 i = 0; i < app_config.mpgr_len; ++i)
    {
      if (p->is_present[i] && p->pages[i].ptr == ptr)
        {
          return i;
        }
    }
  return -1;
}

/**
 * Evicts the page with the greatest time between now and
 * kth last access. Returns the index of the evicted page
 */
u64
mpgr_get_evictable (const memory_pager *p)
{
  memory_pager_assert (p);

  u64 maxK = 0;
  u64 ret = 0;
  ASCOPE (int argmax = -1);

  for (u32 i = 0; i < app_config.mpgr_len; ++i)
    {
      ASSERT (p->is_present[i]);
      u64 this_one = mp_check (&p->pages[i], p->clock);
      if (this_one > maxK)
        {
          maxK = this_one;
          ASCOPE (argmax = i);
          ret = p->pages[i].ptr;
        }
    }

  ASSERT (argmax >= 0);
  ASSERT ((u32)argmax < app_config.mpgr_len);

  return ret;
}

void
mpgr_delete (memory_pager *p, u64 ptr)
{
  int idx = mpgr_check_page_exists (p, ptr);
  ASSERT (idx >= 0);
  ASSERT ((u32)idx < app_config.mpgr_len);
  p->is_present[idx] = 0;
}

void
mpgr_update_pgnum (memory_pager *p, u64 oldptr, u64 newptr)
{
  memory_pager_assert (p);
  ASSERT (newptr != oldptr);

  int idx = mpgr_check_page_exists (p, oldptr);

  ASSERT (idx >= 0);
  ASSERT ((u32)idx < app_config.mpgr_len);
  ASSERT (p->is_present[idx]);

  p->pages[idx].ptr = newptr;
}

///////////////////////////// PAGE TYPES

DEFINE_DBG_ASSERT_I (data_list, data_list, d)
{
  ASSERT (d);
  ASSERT (d->next);
  ASSERT (d->len_num);
  ASSERT (d->len_denom);
  ASSERT (d->data);
}

int
page_read_expect (page *dest, page_type expected, u8 *raw)
{
  ASSERT (dest);
  ASSERT (raw);

  // All pages have a header in the first 8 bytes
  u32 idx = 0;
  u8 *header = &raw[idx];
  idx += sizeof *header;

  if (*header != expected)
    {
      i_log_error ("Expected page type: %d, got page type: %d\n",
                   expected, *header);
      return ERR_INVALID_STATE;
    }

#define advance(vname, type)     \
  do                             \
    {                            \
      vname = (type *)&raw[idx]; \
      idx += sizeof *vname;      \
    }                            \
  while (0)

  switch (expected)
    {
    case PG_DATA_LIST:
      {
        advance (dest->dl.next, i64);
        advance (dest->dl.len_num, u16);
        advance (dest->dl.len_denom, u16);
        advance (dest->dl.data, u8);
        break;
      }
    case PG_INNER_NODE:
      {
        advance (dest->in.nkeys, u16);
        advance (dest->in.leafs, u64);
        advance (dest->in.keys, u64); // TODO - this should be at the end
        break;
      }
    case PG_HASH_PAGE:
      {
        advance (dest->hp.hashes, i64);
        break;
      }
    case PG_HASH_LEAF:
      {
        advance (dest->hl.next, u64);
        advance (dest->hl.nvalues, u16);
        advance (dest->hl.offsets, u16);
        break;
      }
    }

  return SUCCESS;
}

/**
void
page_init (page *dest, page_type type, u8 *raw)
{
  ASSERT (dest);
  ASSERT (type > 0);
  ASSERT (raw);
}
*/
