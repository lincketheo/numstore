#include "paging/types/hash_leaf.h"
#include "dev/assert.h"
#include "ds/strings.h"
#include "intf/stdlib.h"
#include "mm/lalloc.h"
#include "paging/page.h"
#include "paging/types/inner_node.h"
#include "utils/hashing.h"
#include "variables/vhash_fmt.h"

#include <ctype.h> // isprint

DEFINE_DBG_ASSERT_I (hash_leaf, unchecked_hash_leaf, d)
{
  ASSERT (d);
  ASSERT (HL_DATA_OFST + 10 < PAGE_SIZE);
}

DEFINE_DBG_ASSERT_I (hash_leaf, valid_hash_leaf, d)
{
  error e = error_create (NULL);
  ASSERT (hl_validate (d, &e) == SUCCESS);
}

err_t
hl_validate (const hash_leaf *d, error *e)
{
  unchecked_hash_leaf_assert (d);

  /**
   * Check valid header
   */
  if (*d->header != (u8)PG_HASH_LEAF)
    {
      return error_causef (
          e, ERR_CORRUPT,
          "Hash Leaf: expected header: %" PRpgh " but got: %" PRpgh,
          (pgh)PG_HASH_LEAF, *d->header);
    }

  return SUCCESS;
}

void
hl_init_empty (hash_leaf *hl)
{
  /**
   * Just an inconcistency to remember,
   * any time you create a new hash leaf page
   * it will set the first byte to EOF, even if it's
   * a page down the line. This doesn't break anything,
   * but it is slightly inconsistent
   */
  unchecked_hash_leaf_assert (hl);
  *hl->header = PG_HASH_LEAF;
  hl->data[0] = VHFMT_TYPE_EOF;
  valid_hash_leaf_assert (hl);
}

void
hl_set_ptrs (hash_leaf *hl, u8 raw[PAGE_SIZE])
{
  *hl = (hash_leaf){
    .header = (pgh *)&raw[HL_HEDR_OFST],
    .next = (pgno *)&raw[HL_NEXT_OFST],
    .data = (u8 *)&raw[HL_DATA_OFST],
  };
  unchecked_hash_leaf_assert (hl);
}

pgno
hl_get_next (const hash_leaf *hl)
{
  valid_hash_leaf_assert (hl);
  return *hl->next;
}

void
hl_set_next (hash_leaf *hl, pgno next)
{
  valid_hash_leaf_assert (hl);
  *hl->next = next;
}

void
i_log_hl (const hash_leaf *hl)
{
  valid_hash_leaf_assert (hl);

  i_log_info ("=== HASH LEAF PAGE START ===\n");

  i_log_info ("HEADER    : %" PRpgh "\n", *hl->header);
  i_log_info ("NEXT_PAGE : %" PRpgno "\n", *hl->next);

  u8 *bytes = (u8 *)hl->header;
  p_size offset = HL_DATA_OFST;

  i_log_info ("DATA (offset %u to %u):\n", HL_DATA_OFST, PAGE_SIZE);

  while (offset < PAGE_SIZE)
    {
      u32 remain = PAGE_SIZE - offset;
      u32 chunk = remain < 16 ? remain : 16;

      char hex[3 * 16 + 1] = { 0 };
      char asc[16 + 1] = { 0 };

      for (u32 i = 0; i < chunk; ++i)
        {
          u8 b = bytes[offset + i];
          sprintf (&hex[i * 3], "%02x ", b);
          asc[i] = isprint (b) ? b : '.';
        }

      i_log_info ("%04x : %-48s | %s\n", offset, hex, asc);
      offset += chunk;
    }

  i_log_info ("=== HASH LEAF PAGE END ===\n");
}
