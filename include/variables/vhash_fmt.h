#pragma once

#include "ds/strings.h"
#include "intf/mm.h"
#include "intf/types.h"

#define VHFMT_HDR_LEN (5 + sizeof (pgno))
#define VHFMT_TYPE_PRESENT 111
#define VHFMT_TYPE_TOMBSTONE 232
#define VHFMT_TYPE_EOF 152

/**
 * ==================== Type
 * TYPE    [1 byte]
 * ==================== Header
 * VSTRLEN [2 bytes]
 * VSTRLEN [2 bytes]
 * IS_TOMB [1 byte]
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

  enum
  {
    VHFMT_START,
    VHFMT_SCANNING,
    VHFMT_CORRUPT,
    VHFMT_DONE,
    VHFMT_EOF,
  } state;

  union
  {
    u8 header[VHFMT_HDR_LEN];

    struct
    {
      /**
       * Header
       */
      u16 vstrlen;
      u16 tstrlen;
      bool is_tombstone;
      pgno pg0;

      /*
       * data (points to raw)
       */
      char *vstr;
      char *tstr;

      /*
       * Where are we inside data currently?
       */
      p_size vidx;
      p_size tidx;

      u8 *raw; // len = vstrlen + tstrlen
    };
  };
} vread_hash_fmt;

vread_hash_fmt vrhfmt_create (lalloc *alloc);

err_t vrhfmt_read_in (const u8 *src, p_size *nbytes, vread_hash_fmt *dest);

void vrhfmt_free_and_reset (vread_hash_fmt *v);

typedef struct
{
  bool done;

  u8 type_and_header[VHFMT_HDR_LEN + 1];
  u32 hidx;

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

err_t vwhfmt_write_out (u8 *dest, p_size *nbytes, vwrite_hash_fmt *src);
