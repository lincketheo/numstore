#pragma once

#include "intf/types.h" // u16

typedef struct
{
  u16 len;
  char *data;
} string;

string unsafe_cstrfrom (char *cstr);
int strings_all_unique (const string *strs, u32 count);
bool string_equal (const string s1, const string s2);
