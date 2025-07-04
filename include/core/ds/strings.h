#pragma once

#include "core/intf/types.h" // u16

typedef struct
{
  u32 len;
  char *data;
} string;

string unsafe_cstrfrom (char *cstr);
