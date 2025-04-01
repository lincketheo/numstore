#pragma once

#include "types.h"

#include <stdio.h>

///////// FILES
// Return file size or < 0 on error
// Does not maintain location
i64 file_size (int fd);

///////// BYTES
void pretty_print_bytes(FILE* ofp, u8* data, u64 len);
