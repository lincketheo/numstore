#pragma once

#include "ns_memory.h"
#include "ns_types.h"

///////////////////////////////
////////// string

#define MAX_STRLEN 1000
#define PATH_SEPARATOR '/'

typedef struct
{
  char *data;
  ns_size len;
  ns_bool iscstr;
} string;

string string_from_cstr (char *str);

ns_size path_join_len (string prefix, string suffix);

ns_ret_t path_join (string *dest, ns_alloc *a, const string prefix,
                    const string suffix);

bytes stringtobytes (string s);
