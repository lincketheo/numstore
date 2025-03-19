#pragma once

#include "types.hpp"

#define string_assert(s) assert(s); \
  assert((s)->data); \
  assert((s)->len <= (s)->cap)

struct string {
  char* data;
  usize len;
  usize cap;

  inline char* head() { string_assert(this); return &data[len]; }
  inline usize avail() { string_assert(this); return cap - len; }
  result<void> append(const char* data, usize len);
};

result<string> string_create();

void string_free(string* s);
