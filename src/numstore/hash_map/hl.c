#include "numstore/hash_map/hl.h"

#include "core/dev/assert.h"   // TODO
#include "core/errors/error.h" // TODO
#include "core/intf/stdlib.h"  // TODO
#include "core/mm/lalloc.h"    // TODO
#include "core/utils/bounds.h" // TODO
#include "core/utils/macros.h" // TODO

#include "numstore/config.h"                 // TODO
#include "numstore/paging/page.h"            // TODO
#include "numstore/paging/pager.h"           // TODO
#include "numstore/paging/types/hash_leaf.h" // TODO
#include "numstore/variables/variable.h"     // TODO

/**
 * ==================== Header (1 byte)
 * ISEOF       [1 bit]
 * ISPREVTOMB  [1 bit]         - Is previous tombstone
 * ...         [6 bit]
 * ==================== Header
 * VSTRLEN     [2 bytes]        - Length of the variable name
 * TSTRLEN     [2 bytes]        - Length of the type string
 * PGO         [sizeof(pgno)]   - Starting pgno
 * ==================== Data
 * VSTR        [VSTRLEN bytes]  - Variable name string
 * TSTR        [TSTRLEN bytes]  - Type name string
 * ====================
 */

// iseof + vstrlen + tstrlen + pgo + istomb
#define HL_HEADER_LEN (4 + sizeof (pgno)) // Length of upfront header
#define HL_BLEN (MAX_VSTR + MAX_TSTR + HL_HEADER_LEN + 1)

_Static_assert(MAX_VSTR + MAX_TSTR <= UINT16_MAX, "Max vstr and tstr must not exceed max u16");

typedef struct
{
  pager *p;
  const page *pg;
  u32 lidx;
} pgr_stream;

DEFINE_DBG_ASSERT_I (pgr_stream, pgr_stream, p)
{
  ASSERT (p);
  ASSERT (p->p);
  ASSERT (p->pg);
  ASSERT (p->lidx < HL_DATA_LEN);
}

pgr_stream
pgrst_create (pager *p, const page *pg)
{
  pgr_stream ret = {
    .p = p,
    .pg = pg,
    .lidx = 0,
  };

  pgr_stream_assert (&ret);

  return ret;
}

static inline err_t
pgrst_load_next_if_needed (pgr_stream *p, error *e)
{
  pgr_stream_assert (p);

  ASSERT (p->lidx <= HL_DATA_LEN);
  if (p->lidx == HL_DATA_LEN)
    {
      const page *pg = pgr_get (PG_HASH_LEAF, *p->pg->hl.next, p->p, e);
      if (pg == NULL)
        {
          return err_t_from (e);
        }
      p->lidx = 0;
      p->pg = pg;
    }

  return SUCCESS;
}

err_t
pgrst_read (void *dest, u32 nbytes, pgr_stream *p, error *e)
{
  pgr_stream_assert (p);
  ASSERT (nbytes > 0);

  u32 n = 0;

  while (n < nbytes)
    {
      // Next chunk to read
      ASSERT (p->lidx < HL_DATA_LEN);
      u32 next = MIN (HL_DATA_LEN - p->lidx, nbytes - n);

      ASSERT (next > 0); // Loop condition

      // Read into header
      if (dest)
        {
          i_memcpy (dest, &p->pg->hl.data[p->lidx], next);
        }

      n += next;
      p->lidx += next;

      // Load next page if needed
      err_t_wrap (pgrst_load_next_if_needed (p, e), e);
    }

  ASSERT (n == nbytes);

  return SUCCESS;
}

err_t
pgrst_write (const void *src, u32 nbytes, pgr_stream *p, error *e)
{
  pgr_stream_assert (p);
  ASSERT (src);
  ASSERT (nbytes > 0);

  u32 n = 0;

  while (n < nbytes)
    {
      // Next chunk to read
      ASSERT (p->lidx < HL_DATA_LEN);
      u32 next = MIN (HL_DATA_LEN - p->lidx, nbytes - n);

      ASSERT (next > 0); // Loop condition

      // Read into header
      i_memcpy (&p->pg->hl.data[p->lidx], src, next);

      n += next;
      p->lidx += next;

      // Load next page if needed
      err_t_wrap (pgrst_load_next_if_needed (p, e), e);
    }

  return SUCCESS;
}

u8
pgrst_peek (pgr_stream *p)
{
  pgr_stream_assert (p);
  return p->pg->hl.data[p->lidx];
}

void
pgrst_set (pgr_stream *p, u8 byte)
{
  pgr_stream_assert (p);
  page *pg = pgr_make_writable (p->p, p->pg);
  pg->hl.data[p->lidx] = byte;
}

struct hl_s
{
  pager *p; // The pager to load pages
  pgr_stream stream;

  // Header
  union
  {
    u8 header[HL_HEADER_LEN];
    struct
    {
      u16 vstrlen;
      u16 tstrlen;
      pgno pg0;
    };
  };

  // Data
  char *vstr;
  u8 *tstr;

  u8 iseof;
  u8 is_tombstone;

  u8 raw[MAX_VSTR + MAX_TSTR]; // len = vstrlen + tstrlen
};

DEFINE_DBG_ASSERT_I (hl, hl, h)
{
  ASSERT (h);
}

static const char *TAG = "Hash List";

hl *
hl_open (pager *p, error *e)
{
  hl *ret = i_malloc (1, sizeof *ret);
  if (ret == NULL)
    {
      error_causef (
          e, ERR_NOMEM,
          "%s Failed to allocate hash list",
          TAG);
      return NULL;
    }

  *ret = (hl){
    .p = p,

    .iseof = false,
    .vstrlen = 0,
    .tstrlen = 0,
    .pg0 = 0,

    .vstr = NULL,
    .tstr = NULL,

    .is_tombstone = false,
  };

  return ret;
}

void
hl_free (hl *h)
{
  hl_assert (h);
  i_free (h);
}

static inline err_t
parse_and_validate_header (hl *h, error *e)
{
  // Write to another struct to avoid overwriting data
  struct
  {
    u8 iseof;
    u16 vstrlen;
    u16 tstrlen;
    pgno pg0;
  } tmp;

  u32 offset = 0;

  i_memcpy (&tmp.iseof, h->header + offset, sizeof (tmp.iseof));
  offset += sizeof (tmp.iseof);

  i_memcpy (&tmp.vstrlen, h->header + offset, sizeof (tmp.vstrlen));
  offset += sizeof (tmp.vstrlen);

  i_memcpy (&tmp.tstrlen, h->header + offset, sizeof (tmp.tstrlen));
  offset += sizeof (tmp.tstrlen);

  i_memcpy (&tmp.pg0, h->header + offset, sizeof (tmp.pg0));

  // Now assign
  h->iseof = tmp.iseof;
  h->vstrlen = tmp.vstrlen;
  h->tstrlen = tmp.tstrlen;
  h->pg0 = tmp.pg0;

  // Check for valid lengths
  u32 len = 0;
  if (!SAFE_ADD_U16 (&len, h->vstrlen, h->tstrlen)
      || len > MAX_TSTR + MAX_TSTR)
    {
      // Not Arith because all sizes are controlled on entry.
      // There shouldn't be any invalid sizes here
      return error_causef (
          e, ERR_CORRUPT,
          "%s Got hash entry with vstrlen: %u tstrlen: "
          "%u but maximum total size is %u",
          TAG, h->vstrlen, h->tstrlen, MAX_TSTR + MAX_VSTR);
    }

  ASSERT (h->iseof == 0 || h->iseof == 1); // Checked in read_header

  return SUCCESS;
}

static inline void
write_header (hl *h)
{
  struct
  {
    u8 iseof;
    u16 vstrlen;
    u16 tstrlen;
    pgno pg0;
  } tmp;

  tmp.iseof = h->iseof;
  tmp.vstrlen = h->vstrlen;
  tmp.tstrlen = h->tstrlen;
  tmp.pg0 = h->pg0;

  u32 offset = 0;

  i_memcpy (h->header + offset, &tmp.iseof, sizeof (tmp.iseof));
  offset += sizeof (tmp.iseof);

  i_memcpy (h->header + offset, &tmp.vstrlen, sizeof (tmp.vstrlen));
  offset += sizeof (tmp.vstrlen);

  i_memcpy (h->header + offset, &tmp.tstrlen, sizeof (tmp.tstrlen));
  offset += sizeof (tmp.tstrlen);

  i_memcpy (h->header + offset, &tmp.pg0, sizeof (tmp.pg0));
}

static inline void
parse_data (hl *h)
{
  hl_assert (h);

  h->vstr = (char *)h->raw;
  h->tstr = (u8 *)(h->raw + h->vstrlen);
}

err_t
hl_seek (hl *h, var_hash_entry *v, pgno pgn, error *e)
{
  hl_assert (h);

  // Fetch the starting page
  const page *pg = pgr_get (PG_HASH_LEAF, pgn, h->p, e);
  if (pg == NULL)
    {
      return err_t_from (e);
    }

  // Create the pager stream
  h->stream = pgrst_create (h->p, pg);

  return hl_read (h, v, e);
}

bool
hl_eof (hl *h)
{
  hl_assert (h);
  return h->iseof;
}

err_t
hl_read (hl *h, var_hash_entry *dest, error *e)
{
  hl_assert (h);

  // Check if eof
  u8 header = pgrst_peek (&h->stream);
  h->iseof = header & ISEOF;

  // Return early on start is eof
  if (h->iseof)
    {
      return SUCCESS;
    }

  err_t_wrap (pgrst_read (NULL, 1, &h->stream, e), e);                  // 1 byte header
  err_t_wrap (pgrst_read (h->header, HL_HEADER_LEN, &h->stream, e), e); // header
  err_t_wrap (parse_and_validate_header (h, e), e);                     // parse

  // Read in the data
  err_t_wrap (pgrst_read (h->raw, h->vstrlen + h->tstrlen, &h->stream, e), e);
  parse_data (h);

  // Check eof
  header = pgrst_peek (&h->stream);
  h->iseof = 0;
  h->is_tombstone = pgrst_peek (&h->stream) & ISPREVTOMB;

  // Parse into hash entry
  *dest = (var_hash_entry){
    .vstr = (string){
        .data = h->vstr,
        .len = h->vstrlen,
    },
    .tstr = h->tstr,
    .pg0 = h->pg0,
    .tlen = h->tstrlen,
    .is_tombstone = h->is_tombstone,
  };

  return SUCCESS;
}

void
hl_set_tombstone (hl *h)
{
  hl_assert (h);
  u8 header = pgrst_peek (&h->stream);
  header |= ISPREVTOMB;
  pgrst_set (&h->stream, header);
}

err_t
hl_append (hl *h, const variable var, error *e)
{
  hl_assert (h);

  ASSERT (var.vname.len < MAX_VSTR);

  u8 header = pgrst_peek (&h->stream);
  ASSERT (header & ISEOF);
  header &= ~ISEOF;
  err_t_wrap (pgrst_write (&header, 1, &h->stream, e), e);

  var_hash_entry v;
  u8 raw[MAX_VSTR + MAX_TSTR];
  lalloc alloc = lalloc_create_from (raw);

  err_t_wrap (var_hash_entry_create (&v, &var, &alloc, e), e);

  h->vstrlen = v.vstr.len;
  h->tstrlen = v.tlen;
  u32 rlen = h->vstrlen + h->tstrlen;
  h->pg0 = v.pg0;

  // Copy them over - you don't _need_ to do this, but reduces
  // risk of alloc changing TODO - refactor
  i_memcpy (h->raw, v.vstr.data, v.vstr.len);
  i_memcpy (h->raw + v.vstr.len, v.tstr, v.tlen);

  h->vstr = (char *)h->raw;
  h->tstr = (u8 *)(h->raw + v.vstr.len);
  h->is_tombstone = v.is_tombstone;

  write_header (h);

  // Write into pages
  err_t_wrap (pgrst_write (h->header, HL_HEADER_LEN, &h->stream, e), e);
  err_t_wrap (pgrst_write (h->raw, rlen, &h->stream, e), e);

  header = ISEOF;

  // Why would you write a tombstone ?????
  if (h->is_tombstone)
    {
      header |= ISPREVTOMB;
    }
  pgrst_set (&h->stream, header);

  return SUCCESS;
}
