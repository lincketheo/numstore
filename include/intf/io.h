#pragma once

#include "dev/assert.h"
#include "ds/strings.h"
#include "errors/error.h"
#include "intf/types.h"

/////////////////////// Files
typedef struct i_file i_file;

struct i_file
{
  int fd;
};

////////////////// Open / Close
err_t i_open_rw (i_file *dest, const string fname, error *e);
err_t i_close (i_file *fp, error *e);

////////////////// Positional Read / Write
i64 i_pread_some (i_file *fp, void *dest, u64 n, u64 offset, error *e);
i64 i_pread_all (i_file *fp, void *dest, u64 n, u64 offset, error *e);
i64 i_pwrite_some (i_file *fp, const void *src, u64 n, u64 offset, error *e);
err_t i_pwrite_all (i_file *fp, const void *src, u64 n, u64 offset, error *e);

////////////////// Stream Read / Write
i64 i_read_some (i_file *fp, void *dest, u64 nbytes, error *e);
i64 i_read_all (i_file *fp, void *dest, u64 nbytes, error *e);
i64 i_write_some (i_file *fp, const void *src, u64 nbytes, error *e);
err_t i_write_all (i_file *fp, const void *src, u64 nbytes, error *e);

////////////////// Others
err_t i_truncate (i_file *fp, u64 bytes, error *e);
i64 i_file_size (i_file *fp, error *e);
err_t i_remove_quiet (const string fname, error *e);
err_t i_mkstemp (i_file *dest, string tmpl, error *e);
err_t i_unlink (const string name, error *e);

////////////////// Wrappers
err_t i_access_rw (const string fname, error *e);
bool i_exists_rw (const string fname);
err_t i_touch (const string fname, error *e);

////////////////// Memory
void *i_malloc (u32 nelem, u32 size);
void *i_calloc (u32 nelem, u32 size);
void i_free (void *ptr);
