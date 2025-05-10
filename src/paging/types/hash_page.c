#include "paging/types/hash_page.h"
#include "dev/assert.h"
#include "errors/error.h"
#include "intf/stdlib.h"
#include "paging/page.h"
#include "utils/hashing.h"

#define HP_HEDR_OFFSET (0)
#define HP_HASH_OFFSET (HP_HEDR_OFFSET + sizeof (*((hash_page *)0)->hashes))

DEFINE_DBG_ASSERT_I (hash_page, unchecked_hash_page, d)
{
  ASSERT (d);
  ASSERT (d->raw);
  ASSERT (d->rlen > 0);
  ASSERT_PTR_IS_IDX (d->raw, d->header, HP_HEDR_OFFSET);
  ASSERT_PTR_IS_IDX (d->raw, d->hashes, HP_HASH_OFFSET);
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
  i_memset (hp->hashes, 0, hp_hash_len (hp->rlen) * sizeof *hp->hashes);
  valid_hash_page_assert (hp);
}

hash_page
hp_set_ptrs (u8 *raw, p_size len)
{
  ASSERT (raw);
  ASSERT (len > 0);

  hash_page ret;

  // Set pointers
  ret = (hash_page){
    .raw = raw,
    .rlen = len,
    .header = (pgh *)&raw[HP_HEDR_OFFSET],
    .hashes = (pgno *)&raw[HP_HASH_OFFSET],
  };

  unchecked_hash_page_assert (&ret);

  return ret;
}

p_size
hp_hash_len (p_size page_size)
{
  ASSERT (page_size > HP_HASH_OFFSET);
  p_size bytes = page_size - HP_HASH_OFFSET;
  p_size len = bytes / sizeof (pgno);
  ASSERT (len > 0);
  return len;
}

p_size
hp_get_hash_pos (const hash_page *p, const string str)
{
  valid_hash_page_assert (p);
  return fnv1a_hash (str) % hp_hash_len (p->rlen);
}

pgno
hp_get_pgno (const hash_page *p, p_size pos)
{
  valid_hash_page_assert (p);
  ASSERT (pos < hp_hash_len (p->rlen));
  return p->hashes[pos];
}

void
hp_set_hash (hash_page *p, p_size pos, pgno pg)
{
  valid_hash_page_assert (p);
  ASSERT (pos < hp_hash_len (p->rlen));
  p->hashes[pos] = pg;
}
