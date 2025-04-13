#include "paging/memory_page.h"
#include "config.h"
#include "os/stdlib.h"

DEFINE_DBG_ASSERT_I (memory_page, memory_page, p)
{
  ASSERT (p);
}

void
mp_create (memory_page *dest, u64 clock, u64 ptr)
{
  i_memset (&dest->page, 0, PAGE_SIZE);
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
