#include "numstore/paging/types/hash_page.h"

#include "core/dev/assert.h"    // DEFINE_DBG_ASSERT_I
#include "core/intf/stdlib.h"   // i_memset
#include "core/utils/hashing.h" // fnv1a_hash

#include "numstore/paging/page.h" // PG_HASH_PAGE

DEFINE_DBG_ASSERT_I (hash_page, unchecked_hash_page, d)
{
  ASSERT (d);
}

DEFINE_DBG_ASSERT_I (hash_page, valid_hash_page, d)
{
  error e = error_create (NULL);
  ASSERT (hp_validate (d, &e) == SUCCESS);
}

err_t
hp_validate (const hash_page *d, error *e)
{
  unchecked_hash_page_assert (d);

  /**
   * Check valid header
   */
  if (*d->header != (u8)PG_HASH_PAGE)
    {
      return error_causef (
          e, ERR_CORRUPT,
          "Hash Page: expected header: %" PRpgh " but got: %" PRpgh,
          (pgh)PG_HASH_PAGE, *d->header);
    }

  return SUCCESS;
}

void
hp_init_empty (hash_page *hp)
{
  unchecked_hash_page_assert (hp);
  *hp->header = PG_HASH_PAGE;
  i_memset (hp->hashes, 0, HP_NHASHES * sizeof *hp->hashes);
  valid_hash_page_assert (hp);
}

void
hp_set_ptrs (hash_page *hp, u8 raw[PAGE_SIZE])
{
  *hp = (hash_page){
    .header = (pgh *)&raw[HP_HEDR_OFST],
    .hashes = (pgno *)&raw[HP_HASH_OFST],
  };
  unchecked_hash_page_assert (hp);
}

p_size
hp_get_hash_pos (const hash_page *p, const string str)
{
  valid_hash_page_assert (p);
  return fnv1a_hash (str) % HP_NHASHES;
}

pgno
hp_get_pgno (const hash_page *p, p_size pos)
{
  valid_hash_page_assert (p);
  ASSERT (pos < HP_NHASHES);
  return p->hashes[pos];
}

void
hp_set_hash (hash_page *p, p_size pos, pgno pg)
{
  valid_hash_page_assert (p);
  ASSERT (pos < HP_NHASHES);
  p->hashes[pos] = pg;
}

void
i_log_hp (const hash_page *hp)
{
  valid_hash_page_assert (hp);

  i_log_info ("=== HASH PAGE START ===\n");

  i_log_info ("HEADER : %" PRpgh "\n", *hp->header);
  i_log_info ("LEN    : %u\n", HP_NHASHES);

  for (u32 i = 0; i < HP_NHASHES; ++i)
    {
      if (hp->hashes[i])
        {
          char line[128] = { 0 };
          int pos = 0;
          pos += snprintf (
              &line[pos], sizeof (line) - pos,
              "[%03u] = %" PRpgno "  ",
              i,
              hp->hashes[i]);
          i_log_info ("%s\n", line);
        }
    }

  i_log_info ("=== HASH PAGE END ===\n");
}
