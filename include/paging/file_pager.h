#pragma once

#include "intf/io.h"
#include "intf/types.h"

typedef struct
{
  u32 page_size;
  u64 npages;
  u32 header_size;
  i_file f;
} file_pager;

typedef struct
{
  i_file f;
  u32 header_size;
  u32 page_size;
} fpgr_params;

/**
 * Returns:
 *   - ERR_INVALID_STATE if it senses the file is in an
 *   invalid state
 */
err_t fpgr_create (file_pager *dest, fpgr_params);

/**
 * Allocates a new page and stores result in pgno_dest
 * Returns:
 *   - Forwards errors from i_truncate
 */
err_t fpgr_new (file_pager *p, u64 *pgno_dest);

/**
 * Commits data pointed to by src (assuming page_size) to page pgno
 * Returns:
 *  - Forwards errors from i_write_all
 */
err_t fpgr_commit (file_pager *p, const u8 *src, u64 pgno);

/**
 * Deletes page at pgno.
 * TODO - This doesn't do anything yet because I don't have a
 * good page deletion management
 */
err_t fpgr_delete (file_pager *p, u64 pgno);

/**
 * Fetches page. Expect page to exist
 * Returns:
 *   - INVALID_STATE - page doesn't exist
 *   - ERR_IO - read fails
 */
err_t fpgr_get_expect (file_pager *p, u8 *dest, u64 pgno);
