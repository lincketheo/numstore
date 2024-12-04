#pragma once

#include "ns_memory.h"
#include <stdlib.h>

///////////////////////////////
////////// Buffers

typedef struct
{
  void *data;
  size_t cap;
  size_t len;
} u8_qbuf;

typedef struct
{
  void *data;
  size_t cap;
  size_t len;
} u8_sbuf;

///////////////////////////////
////////// srange

typedef struct
{
  size_t start;
  size_t end;
  size_t stride;
  int isinf;
} srange;

#define srange_ASSERT(s)                                                      \
  ASSERT (s.start <= s.end);                                                  \
  ASSERT (s.stride > 0)

///////////////////////////////
////////// string

#define MAX_STRLEN 1000
#define PATH_SEPARATOR '/'

typedef struct
{
  const char *data;
  size_t len;
  int iscstr;
} string;

string string_from_cstr (const char *str);

size_t path_join_len (string prefix, string suffix);

string path_join (ns_alloc *a, string prefix, string suffix);
