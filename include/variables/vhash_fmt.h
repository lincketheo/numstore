#pragma once

#include "config.h"             // MAX_VSTR
#include "ds/strings.h"         // string
#include "intf/types.h"         // u16
#include "mm/lalloc.h"          // lalloc
#include "variables/variable.h" // var_hash_entry

#define VHFMT_HDR_LEN (5 + sizeof (pgno)) // Length of the header
#define VHFMT_TYPE_PRESENT 111            // Valid variable
#define VHFMT_TYPE_TOMBSTONE 232          // Deleted variable
#define VHFMT_TYPE_EOF 152                // End of the hash list

/**
 * A Variable hash format reader / writer
 *
 * A variable in hash format looks like this:
 * ==================== Type
 * TYPE    [1 byte]         - Tombstone, eof, or present (else corrupt)
 * ==================== Header
 * VSTRLEN [2 bytes]        - Length of the variable name
 * TSTRLEN [2 bytes]        - Length of the type string
 * PGO     [sizeof(pgno)]   - Starting pgno
 * ==================== Data
 * VSTR    [VSTRLEN bytes]  - Variable name string
 * TSTR    [TSTRLEN bytes]  - Type name string
 * ====================
 */

/**
 * First, the reader. The reader takes in a constant byte array [src]
 * and reads into itself building the variable string
 * with each read iteratively. Once it enters the Done state,
 * [vstr] and [tstr] are pointers to the variable name string
 * and type string and the header is set
 */
typedef struct
{
  u32 pos; // Position inside this variable (byte number)

  /**
   * START -> SCANNING | CORRUPT | EOF
   * SCANNING -> DONE
   * EOF | DONE -> Do not call read (UNREACHABLE)
   */
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
    // Reads in the header first, then parses it
    u8 header[VHFMT_HDR_LEN];

    // The actual data
    struct
    {
      // Header
      u16 vstrlen;
      u16 tstrlen;
      bool is_tombstone;
      pgno pg0;

      // Data - null if tombstone
      char *vstr;
      u8 *tstr;

      // Where inside each string are we located
      p_size vidx; // if < vstrlen, tidx == 0
      p_size tidx;

      u8 raw[MAX_VSTR + MAX_TSTR]; // len = vstrlen + tstrlen
    };
  };
} vread_hash_fmt;

vread_hash_fmt vrhfmt_create (void);

/**
 * After a call to read, the vread_hash_fmt state may change
 * Errors:
 *   - ERR_CORRUPT
 *       - Unknown header value
 *       - Invalid header
 *          vstrlen == 0 || > MAX_VSTR
 *          tstrlen == 0 || > MAX_TSTR
 *          pg0 == 0
 */
err_t vrhfmt_read_in (
    const u8 *src,        // Source data to read from
    p_size *nbytes,       // bytes to read (sets it to actual)
    vread_hash_fmt *dest, // Write into here
    error *e              // Error accumulator
);

/**
 * Consumes a reader (must be in the done state)
 */
var_hash_entry vrhfmt_consume (vread_hash_fmt *fmt);

/**
 * Next, writer already stores
 */
typedef struct
{
  bool done;

  u8 type_and_header[VHFMT_HDR_LEN + 1];
  u32 hidx; // Where in [type_and_header] are we?

  const var_hash_entry src;

  // Where in each string are we?
  p_size vidx;
  p_size tidx;
} vwrite_hash_fmt;

vwrite_hash_fmt vwhfmt_create (const var_hash_entry v);

/**
 * Weird behavior:
 *   - Expects the first byte in dest to be VHFMT_TYPE_EOF - So that
 *     we're only writing to a previous EOF. It's a little
 *     puzzle piece to fit these nodes together
 *
 * Errors:
 *   - ERR_CORRUPT - Byte 0 is not EOF
 */
err_t vwhfmt_write_out (
    u8 *dest,             // Where to write out to
    p_size *nbytes,       // number of bytes to write
    vwrite_hash_fmt *src, // The thing we're writing
    error *e              // Error accumulator
);
