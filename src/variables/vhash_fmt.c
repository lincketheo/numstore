#include "variables/vhash_fmt.h"
#include "errors/error.h"
#include "intf/stdlib.h"
#include "mm/lalloc.h"

#define MIN(a, b) ((a) < (b) ? (a) : (b))

vread_hash_fmt
vrhfmt_create (lalloc *alloc)
{
  return (vread_hash_fmt){
    .pos = 0,
    .alloc = alloc,
    .state = VHFMT_START,
  };
}

void
vrhfmt_reset (vread_hash_fmt *v)
{
  lalloc_reset (v->alloc);
}

static err_t
vrhfmt_parse_header (vread_hash_fmt *v, error *e)
{
  ASSERT (v->pos == VHFMT_HDR_LEN + 1);
  ASSERT (v->state == VHFMT_SCANNING);

  u16 vstrlen, tstrlen;
  u8 is_tombstone;
  pgno pg0;

  i_memcpy (&vstrlen, v->header + 0, sizeof vstrlen);
  i_memcpy (&tstrlen, v->header + 2, sizeof tstrlen);
  i_memcpy (&is_tombstone, v->header + 4, sizeof is_tombstone);
  i_memcpy (&pg0, v->header + 5, sizeof pg0);

  /**
   * Zero everything out
   */
  {
    u32 pos = v->pos;
    lalloc *alloc = v->alloc;
    i_memset (v, 0, sizeof *v);
    v->pos = pos;
    v->alloc = alloc;
  }

  /**
   * Require strs > 0 and no magic pages
   */
  if (vstrlen == 0 || tstrlen == 0 || pg0 == 0)
    {
      v->state = VHFMT_CORRUPT;
      return error_causef (
          e, ERR_CORRUPT,
          "VHash Format: "
          "Invalid variable header");
    }

  v->state = VHFMT_SCANNING;
  v->vstrlen = vstrlen;
  v->tstrlen = tstrlen;
  v->is_tombstone = is_tombstone;
  v->pg0 = pg0;

  /**
   * Allocate strings
   */
  if (!v->is_tombstone)
    {
      u8 *raw = lmalloc (v->alloc, vstrlen + tstrlen, 1);
      if (raw == NULL)
        {
          return error_causef (
              e, ERR_NOMEM,
              "VHash Format: "
              "Failed to allocate vhash format string");
        }

      v->raw = raw;
      v->vstr = (char *)v->raw;
      v->tstr = v->raw + vstrlen;
    }

  return SUCCESS;
}

err_t
vrhfmt_read_in (
    const u8 *src,
    p_size *nbytes,
    vread_hash_fmt *dest,
    error *e)
{
  ASSERT (*nbytes > 0);

  err_t ret = SUCCESS;     // Return code
  p_size next;             // Next write block
  p_size read = 0;         // Total bytes read
  p_size toread = *nbytes; // Total bytes to read

  /**
   * Read in the type byte
   */
  if (dest->pos == 0)
    {
      ASSERT (dest->state == VHFMT_START);

      u8 type = src[dest->pos++];
      read += 1;

      switch (type)
        {
        case VHFMT_TYPE_PRESENT:
          {
            dest->is_tombstone = false;
            dest->state = VHFMT_SCANNING;
            break;
          }
        case VHFMT_TYPE_TOMBSTONE:
          {
            dest->is_tombstone = true;
            dest->state = VHFMT_SCANNING;
            break;
          }
        case VHFMT_TYPE_EOF:
          {
            dest->state = VHFMT_EOF;
            ret = SUCCESS;
            goto theend;
          }
        default:
          {
            dest->state = VHFMT_CORRUPT;
            ret = error_causef (
                e, ERR_CORRUPT,
                "Unexpected hash leaf variable header: %d", type);
            goto theend;
          }
        }
    }

  ASSERT (dest->state == VHFMT_SCANNING);
  ASSERT (dest->pos > 0);

  /**
   * 1 plus because header only includes the stuff after
   * the type byte
   */
  if (dest->pos < 1 + VHFMT_HDR_LEN)
    {
      /**
       * Read maximum into header
       */
      next = MIN (VHFMT_HDR_LEN + 1 - dest->pos, toread);
      if (next > 0)
        {
          i_memcpy (dest->header + dest->pos - 1, src + read, next);
          dest->pos += next;

          read += next;
          toread -= next;
        }

      /**
       * Header's done - parse it
       */
      if (dest->pos == VHFMT_HDR_LEN + 1)
        {
          err_t_wrap (vrhfmt_parse_header (dest, e), e);
        }

      /**
       * Otherwise we still have more to do
       * on the header
       */
      else
        {
          goto theend;
        }
    }

  ASSERT (dest->pos >= VHFMT_HDR_LEN + 1);
  ASSERT (dest->state == VHFMT_SCANNING);

  /**
   * Read in vstr if it's not done yet
   */
  if (dest->vidx < dest->vstrlen)
    {
      ASSERT (dest->tidx == 0);
      next = MIN (dest->vstrlen - dest->vidx, toread);
      if (next > 0)
        {
          if (!dest->is_tombstone)
            {
              i_memcpy (dest->vstr + dest->vidx, src + read, next);
            }
          dest->vidx += next;
          dest->pos += next;

          read += next;
          toread -= next;
        }

      if (toread == 0)
        {
          goto theend;
        }
    }

  /**
   * Read in tstr if it's not done and vstr _is_ done
   */
  if (dest->vidx == dest->vstrlen && dest->tidx < dest->tstrlen)
    {
      next = MIN (dest->tstrlen - dest->tidx, toread);
      if (next > 0)
        {
          if (!dest->is_tombstone)
            {
              i_memcpy (dest->tstr + dest->tidx, src + read, next);
            }
          dest->tidx += next;
          dest->pos += next;

          read += next;
          toread -= next;
        }
    }

  if (dest->tidx == dest->tstrlen)
    {
      ASSERT (dest->pos == 1 + VHFMT_HDR_LEN + dest->tstrlen + dest->vstrlen);
      dest->state = VHFMT_DONE;
    }

theend:
  *nbytes = read;
  return ret;
}

vwrite_hash_fmt
vwhfmt_create (var_hash_entry params)
{
  vwrite_hash_fmt w = {
    .done = false,
    .hidx = 0,

    .vstr = params.vstr,
    .vstrlen = params.vlen,

    .tstr = params.tstr,
    .tstrlen = params.tlen,

    .vidx = 0,
    .tidx = 0,
  };

  u8 *p = w.type_and_header;

  p[0] = (u8)VHFMT_TYPE_PRESENT;
  i_memcpy (p + 1, &w.vstrlen, sizeof w.vstrlen);
  i_memcpy (p + 3, &w.tstrlen, sizeof w.tstrlen);
  p[5] = 0; // is_tombstone
  i_memcpy (p + 6, &params.pg0, sizeof params.pg0);

  return w;
}

err_t
vwhfmt_write_out (
    u8 *dest,
    p_size *nbytes,
    vwrite_hash_fmt *src,
    error *e)
{
  ASSERT (*nbytes > 0);

  p_size next;
  p_size towrite = *nbytes;
  p_size written = 0;

  /**
   * The puzzle piece -
   * writer only writes to a previous
   * eof type
   */
  if (src->hidx == 0)
    {
      if (dest[0] != VHFMT_TYPE_EOF)
        {
          return error_causef (
              e, ERR_CORRUPT,
              "Expected header VHFMT_TYPE_EOF (%d) "
              "to start next variable, but got: %d",
              VHFMT_TYPE_EOF, dest[0]);
        }
    }

  /**
   * Write out the type_and_header
   */
  if (src->hidx < VHFMT_HDR_LEN + 1)
    {
      next = MIN (VHFMT_HDR_LEN + 1 - src->hidx, towrite);
      if (next > 0)
        {
          i_memcpy (
              dest + written,
              src->type_and_header + src->hidx,
              next);
          src->hidx += next;
          written += next;
          towrite -= next;
        }

      if (towrite == 0)
        {
          goto theend;
        }
    }

  ASSERT (src->hidx <= VHFMT_HDR_LEN + 1);

  /**
   * Write out vstr if type_and_header is done
   */
  if (src->hidx == VHFMT_HDR_LEN + 1 && src->vidx < src->vstrlen)
    {
      next = MIN (src->vstrlen - src->vidx, towrite);
      if (next > 0)
        {
          i_memcpy (dest + written, src->vstr + src->vidx, next);
          src->vidx += next;
          written += next;
          towrite -= next;
        }

      if (towrite == 0)
        {
          goto theend;
        }
    }

  /**
   * Write out tstr if vstr is done
   */
  if (src->vidx == src->vstrlen && src->tidx < src->tstrlen)
    {
      next = MIN (src->tstrlen - src->tidx, towrite);
      if (next > 0)
        {
          memcpy (dest + written, src->tstr + src->tidx, next);
          src->tidx += next;
          written += next;
          towrite -= next;
        }
    }

  /**
   * Write out the last EOF byte
   */
  if (src->tidx == src->tstrlen && !src->done)
    {
      next = MIN (1, towrite);
      if (next > 0)
        {
          u8 eof = VHFMT_TYPE_EOF;
          i_memcpy (dest + written, &eof, 1);
          written += next;
          towrite -= next;
          src->done = true;
        }
    }

theend:
  ASSERT (written <= *nbytes);
  ASSERT (towrite <= *nbytes);
  *nbytes = written;
  return SUCCESS;
}
