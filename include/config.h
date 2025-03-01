#pragma once

#include "types.h"

typedef struct
{
  u32 page_size;
  u32 mpgr_len;
} config;

static const config app_config = {
  .page_size = 4096,
  .mpgr_len = 100,
};
