#include "variables/vhash_fmt.h"
#include "dev/errors.h"
#include "intf/stdlib.h"

#define MIN(a, b) ((a) < (b) ? (a) : (b))

vread_hash_fmt
vrhfmt_create (lalloc *alloc)
{
  return (vread_hash_fmt){
    .pos = 0,
    .alloc = alloc,
  };
}

err_t
vrhfmt_parse_header (vread_hash_fmt *v)
{
  ASSERT (v->pos == VHASH_HEADER_LEN);

  u8 type;
  u16 vstrlen, tstrlen;
  pgno pg0;

  i_memcpy (&type, v->header + 0, sizeof type);
  i_memcpy (&vstrlen, v->header + 1, sizeof vstrlen);
  i_memcpy (&tstrlen, v->header + 3, sizeof tstrlen);
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

  switch (type)
    {
    case (u8)VHFMT_EOF:
      {
        v->type = VHFMT_EOF;
        return SUCCESS;
      }
    case (u8)VHFMT_TOMBSTONE:
    case (u8)VHFMT_PRESENT:
      {
        /**
         * Require strs > 0 and no magic pages
         */
        if (vstrlen == 0 || tstrlen == 0 || pg0 == 0)
          {
            return ERR_INVALID_STATE;
          }

        v->vstrlen = vstrlen;
        v->tstrlen = tstrlen;
        v->pg0 = pg0;
        v->type = type;
        break;
      }
    default:
      {
        v->type = VHFMT_UNKNOWN;
        return ERR_INVALID_STATE;
      }
    }

  /**
   * Allocate strings
   */
  if (v->type == VHFMT_PRESENT)
    {
      v->raw = lmalloc (v->alloc, vstrlen + tstrlen);
      if (!v->raw)
        {
          lfree (v->alloc, v->raw);
          return ERR_NOMEM;
        }
      v->vstr = (char *)v->raw;
      v->tstr = (char *)v->raw + vstrlen;
    }

  return SUCCESS;
}

err_t
vrhfmt_read_in (const u8 *src, p_size *nbytes, vread_hash_fmt *dest)
{
  err_t ret = SUCCESS;     // Return code
  p_size next;             // Next write block
  p_size read = 0;         // Total bytes read
  p_size toread = *nbytes; // Total bytes to read

  if (dest->pos < VHASH_HEADER_LEN)
    {
      /**
       * Read maximum into header
       */
      next = MIN (VHASH_HEADER_LEN - dest->pos, toread);
      if (next > 0)
        {
          i_memcpy (dest->header + dest->pos, src + read, next);
          dest->pos += next;

          read += next;
          toread -= next;
        }

      /**
       * Header's done - parse it
       */
      if (dest->pos == VHASH_HEADER_LEN)
        {
          ret = vrhfmt_parse_header (dest);
          if (ret)
            {
              goto theend;
            }
        }

      /**
       * Otherwise we still have more to do
       */
      else
        {
          goto theend;
        }
    }

  ASSERT (dest->pos >= VHASH_HEADER_LEN);

  /**
   * Read in vstr if it's not done yet
   */
  if (dest->vidx < dest->vstrlen)
    {
      ASSERT (dest->tidx == 0);
      next = MIN (dest->vstrlen - dest->vidx, toread);
      if (next > 0)
        {
          i_memcpy (dest->vstr + dest->vidx, src + read, next);
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
          i_memcpy (dest->tstr + dest->tidx, src + read, next);
          dest->tidx += next;
          dest->pos += next;

          read += next;
          toread -= next;
        }
    }

theend:
  *nbytes = read;
  return ret;
}

bool
vrhfmt_done (vread_hash_fmt *v)
{
  if (v->pos < VHASH_HEADER_LEN)
    {
      return false;
    }
  u32 target = VHASH_HEADER_LEN + v->vstrlen + v->tstrlen;
  ASSERT (v->pos <= target);
  return v->pos == target;
}

void
vrhfmt_free_and_reset (vread_hash_fmt *v)
{
  if (v->raw)
    {
      lfree (v->alloc, v->raw);
    }

  {
    lalloc *alloc = v->alloc;
    i_memset (v, 0, sizeof *v);
    v->alloc = alloc;
  }
}

vwrite_hash_fmt
vwhfmt_create (vwhfmt_params params)
{
  vwrite_hash_fmt w = {
    .pos = 0,
    .vidx = 0,
    .tidx = 0,
    .vstr = params.vstr,
    .tstr = params.tstr,
  };

  u8 *p = w.header;
  p[0] = (u8)VHFMT_PRESENT;
  u16 vlen = (u16)params.vstr.len;
  u16 tlen = (u16)params.tstr.len;
  memcpy (p + 1, &vlen, sizeof vlen);
  memcpy (p + 3, &tlen, sizeof tlen);
  memcpy (p + 5, &params.pg0, sizeof params.pg0);

  return w;
}

void
vwhfmt_write_out (u8 *dest, p_size *nbytes, vwrite_hash_fmt *src)
{
  p_size next;
  p_size towrite = *nbytes;
  p_size written = 0;

  /**
   * Write out the header
   */
  if (src->pos < VHASH_HEADER_LEN)
    {
      next = MIN (VHASH_HEADER_LEN - src->pos, towrite);
      if (next > 0)
        {
          i_memcpy (dest + written, src->header + src->pos, next);
          src->pos += next;
          written += next;
          towrite -= next;
        }

      if (towrite == 0)
        {
          goto theend;
        }
    }

  /**
   * Write out vstr if header is done
   */
  if (src->pos >= VHASH_HEADER_LEN && src->vidx < src->vstr.len)
    {
      next = MIN (src->vstr.len - src->vidx, towrite);
      if (next > 0)
        {
          i_memcpy (dest + written, src->vstr.data + src->vidx, next);
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
  if (src->vidx == src->vstr.len && src->tidx < src->tstr.len)
    {
      next = MIN (src->tstr.len - src->tidx, towrite);
      if (next > 0)
        {
          memcpy (dest + written, src->tstr.data + src->tidx, next);
          src->tidx += next;
          written += next;
          towrite -= next;
        }
    }

theend:
  *nbytes = written;
}

bool
vwhfmt_done (vwrite_hash_fmt *src)
{
  return src->pos == VHASH_HEADER_LEN
         && src->vidx == src->vstr.len
         && src->tidx == src->tstr.len;
}
