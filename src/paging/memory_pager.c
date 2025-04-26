#include "paging/memory_pager.h"
#include "dev/errors.h"
#include "dev/testing.h"
#include "intf/mm.h"
#include "intf/stdlib.h"

///////////////////////////// MEMORY PAGE

DEFINE_DBG_ASSERT_I (memory_page, memory_page, p)
{
  ASSERT (p);
  ASSERT (p->data);
}

void
mp_create (memory_page *dest, u8 *data, u32 dlen, u64 pgno)
{
  dest->data = data;
  i_memset (dest->data, 0, dlen);
  dest->data = data;
  dest->pgno = pgno;
}

TEST (mp_create)
{
  u8 _page[2048];

  memory_page m;

  mp_create (&m, _page, 2048, 42);

  // fields initialized correctly
  test_assert_int_equal ((int)m.pgno, 42);

  // page memory zeroed out (check first and last byte)
  test_assert_int_equal (_page[0], 0);
  test_assert_int_equal (_page[2048 - 1], 0);
}

///////////////////////////// MEMORY PAGER

DEFINE_DBG_ASSERT_I (memory_pager, memory_pager, p)
{
  ASSERT (p);
  ASSERT (p->pages);
  ASSERT (p->len > 0);
  for (u32 i = 0; i < p->len; ++i)
    {
      memory_page_assert (&p->pages[i]);
    }
}

err_t
mpgr_create (memory_pager *dest, mpgr_params params)
{
  ASSERT (dest);
  ASSERT (params.len > 0);

  memory_page *pages = lmalloc (
      params.alloc,
      params.len * sizeof *pages);

  if (pages == NULL)
    {
      return ERR_NOMEM;
    }

  u8 *page_block = lmalloc (
      params.alloc,
      params.len * params.page_size);

  if (page_block == NULL)
    {
      // TODO - free here
      return ERR_NOMEM;
    }

  // Initialize the pages
  for (u32 i = 0; i < params.len; ++i)
    {
      pages[i].data = page_block + i * params.page_size;
    }

  dest->pages = pages;
  dest->len = params.len;
  dest->idx = 0;

  memory_pager_assert (dest);

  return SUCCESS;
}

u8 *
mpgr_new (memory_pager *p, u64 pgno)
{
  memory_pager_assert (p);

  for (u32 i = 0; i < p->len; ++i)
    {

      memory_page *mp = &p->pages[p->idx];
      p->idx = (p->idx + 1) % p->len;

      if (!mp->is_present)
        {
          mp->pgno = pgno;
          mp->is_present = 1;
          return mp->data;
        }
    }

  return NULL;
}

bool
mpgr_is_full (const memory_pager *p)
{
  memory_pager_assert (p);

  u32 idx = p->idx;

  for (u32 i = 0; i < p->len; ++i)
    {
      memory_page *mp = &p->pages[idx];
      idx = (p->idx + 1) % p->len;

      if (!mp->is_present)
        {
          return false;
        }
    }

  return true;
}

u8 *
mpgr_get (const memory_pager *p, u64 pgno)
{
  memory_pager_assert (p);

  for (u32 i = 0; i < p->len; ++i)
    {

      memory_page *mp = &p->pages[(i + p->idx) % p->len];

      if (mp->is_present && mp->pgno == pgno)
        {
          return mp->data;
        }
    }

  return NULL;
}

u64
mpgr_get_evictable (const memory_pager *p)
{
  memory_pager_assert (p);
  memory_page *mp = &p->pages[p->idx];
  ASSERT (mp->is_present);
  return mp->pgno;
}

void
mpgr_evict (memory_pager *p, u64 pgno)
{
  memory_pager_assert (p);

  for (u32 i = 0; i < p->len; ++i)
    {

      memory_page *mp = &p->pages[p->idx];

      if (mp->is_present && mp->pgno == pgno)
        {
          mp->is_present = 0;
          return;
        }

      p->idx = (p->idx + 1) % p->len;
    }
  ASSERT (0);
}

TEST (mpgr_evict)
{
  // TODO
}
