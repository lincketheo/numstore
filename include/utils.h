#pragma once

#include "types.h"

#include <stdio.h>

///////// FILES
int file_create_from (const char *fname, void *data, usize lenb);

int write_all (int fd, const void *src, usize blen);

ssize read_all (int fd, void *dest, usize blen);

///////// BYTES
void pretty_print_bytes(FILE* ofp, u8* data, usize len);
