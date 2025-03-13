#pragma once

#include "types.hpp"

///////// FILES
int file_create_from (const char *fname, void *data, usize lenb);

int write_all (int fd, const void *src, usize blen);

ssize read_all (int fd, void *dest, usize blen);
