#pragma once

#include "dev/assert.h"
#include "sds.h"
#include "types.h"

#ifndef NDEBUG
void io_log_stats (void);
#endif

/////////////////////// Files
typedef struct i_file i_file;

DEFINE_DBG_ASSERT_H (i_file, i_file, p);
i_file *i_open (const string fname, int read, int write);
int i_close (i_file *fp);
i64 i_read_some (i_file *fp, void *dest, u64 n, u64 offset);
i64 i_read_all (i_file *fp, void *dest, u64 n, u64 offset);
i64 i_write_some (i_file *fp, const void *src, u64 n, u64 offset);
int i_write_all (i_file *fp, const void *src, u64 n, u64 offset);
int i_truncate (i_file *fp, u64 bytes);
i64 i_file_size (i_file *fp);
int i_remove_quiet (const string fname);
