#pragma once

#include "ds/strings.h"
#include "intf/mm.h"
#include "intf/types.h"

#define VHASH_HEADER_LEN (5 + sizeof (pgno))

/**
 * ==================== Header
 * TYPE    [1 byte]
 * VSTRLEN [2 bytes]
 * VSTRLEN [2 bytes]
 * PGO     [sizeof(pgno)]
 * ==================== Data
 * VSTR    [VSTRLEN bytes]
 * TSTR    [TSTRLEN bytes]
 * ====================
 */

typedef struct
{
  u32 pos;
  lalloc *alloc; // Allocates [raw]

  union
  {
    u8 header[VHASH_HEADER_LEN];

    struct
    {
      // From the header
      u16 vstrlen;
      u16 tstrlen;
      pgno pg0;
      enum
      {
        VHFMT_TOMBSTONE,
        VHFMT_EOF,
        VHFMT_PRESENT,
        VHFMT_UNKNOWN,
      } type;

      // The data (points to raw)
      char *vstr;
      char *tstr;

      // Where are we currently?
      p_size vidx;
      p_size tidx;

      u8 *raw; // len = vstrlen + tstrlen
    };
  };
} vread_hash_fmt;

vread_hash_fmt vrhfmt_create (lalloc *alloc);
err_t vrhfmt_read_in (const u8 *src, p_size *nbytes, vread_hash_fmt *dest);
bool vrhfmt_done (vread_hash_fmt *v);
void vrhfmt_free_and_reset (vread_hash_fmt *v);

typedef struct
{
  u32 pos;

  u8 header[VHASH_HEADER_LEN];

  const string vstr;
  const string tstr;

  p_size vidx;
  p_size tidx;
} vwrite_hash_fmt;

typedef struct
{
  const string vstr;
  const string tstr;
  pgno pg0;
} vwhfmt_params;

vwrite_hash_fmt vwhfmt_create (vwhfmt_params params);
void vwhfmt_write_out (u8 *dest, p_size *nbytes, vwrite_hash_fmt *src);
bool vwhfmt_done (vwrite_hash_fmt *v);
