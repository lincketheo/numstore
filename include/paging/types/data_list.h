#pragma once

#include "dev/assert.h"
#include "os/types.h"

typedef struct
{
  u8 *page;

  u8 *header;
  u64 *next;
  u64 *idx_num;
  u64 *idx_den;
  u64 *len;

  u8 *data;
} data_list;

DEFINE_DBG_ASSERT_H (data_list, data_list, d);

int dl_read_page (data_list *dest, u8 *page);

void dl_read_and_init_page (data_list *dest, u8 *page);
