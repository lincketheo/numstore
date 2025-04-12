#pragma once

#define ASSERT(expr) do { if (!(expr)) { *(volatile int*)0 = 1; } } while(0)

#define DEFINE_ASSERT(type, name, variable)                                         \
__attribute__((unused)) static inline int name##_valid(const type* variable); \
__attribute__((unused)) static inline void name##_assert(const type* p) \
{                                                                       \
  ASSERT(p);                                                            \
  ASSERT(name##_valid(p));                                              \
} \
__attribute__((unused)) static inline int name##_valid(const type* variable)
