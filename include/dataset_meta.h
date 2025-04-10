#pragma once 

#include "ns_assert.h"
#include "types.h"

#pragma pack(push, 1)
typedef struct
{
  page_ptr data_start;
} dataset_meta;
#pragma pack(pop)

static inline int dataset_meta_valid(const dataset_meta* d) {
  return d->data_start > 0;
}

DEFINE_ASSERT(dataset_meta, dataset_meta)
