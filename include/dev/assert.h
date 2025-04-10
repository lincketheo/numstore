#pragma once

#include <assert.h>

#define todo_assert(expr)          \
  LOG_WARN("TODO!: Fix %s", #expr) \
  assert(expr)

#define DEFINE_ASSERT(type, name)                                         \
  __attribute__((unused)) static inline void name##_assert(const type* p) \
  {                                                                       \
    assert(p);                                                            \
    assert(name##_valid(p));                                              \
  }
