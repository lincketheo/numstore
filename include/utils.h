#pragma once

#include "types.h"

#include <stdio.h>

///////// FILES
int file_create_from (const char *fname, void *data, u64 lenb);

int write_all (int fd, const void *src, u64 blen);

i64 read_all (int fd, void *dest, u64 blen);

///////// BYTES
void pretty_print_bytes(FILE* ofp, u8* data, u64 len);
