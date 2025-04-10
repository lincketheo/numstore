#pragma once

#include "common/types.h"

typedef struct rads_file_s rads_file;

// Check if file is valid
int rads_file_valid(const rads_file* f);

DEFINE_ASSERT(rads_file, rads_file)

// Get the file size in bytes
i64 rads_file_size(rads_file* f);
