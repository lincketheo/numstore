#pragma once

#include "common/types.h"

typedef struct
{
  u8 *page;

  u8 *header;
  u64 *next;
  u8 *size;
  u8 *len;

  u8 *data;
} index_list;

DEFINE_DBG_ASSERT (index_list, index_list, i);

int il_read_page (index_list *dest, u8 *page);
