#pragma once

#include <stdint.h>

#include <stdint.h>

#define CBUF_DEF(name, type, size)                     \
  typedef struct {                                     \
    type data[size];                                   \
    int head, tail;                                    \
    int full;                                          \
  } name;                                              \
                                                       \
  static inline int name##_is_valid(const name##* cb)  \
  {                                                    \
    if (cb == NULL)                                    \
      return 0;                                        \
    if (cb->head < 0 || cb->head >= size)              \
      return 0;                                        \
    if (cb->tail < 0 || cb->tail >= size)              \
      return 0;                                        \
    if (cb->full && (cb->head != cb->tail))            \
      return 0;                                        \
    return 1;                                          \
  }                                                    \
  DEFINE_ASSERT(name, name)                            \
                                                       \
  static inline void name##_init(name##* cb)           \
  {                                                    \
    cb->head = 0;                                      \
    cb->tail = 0;                                      \
    cb->full = 0;                                      \
    name##_assert(cb);                                 \
  }                                                    \
                                                       \
  static inline int name##_len(const name##* cb)       \
  {                                                    \
    name##_assert(cb);                                 \
    if (cb->full) {                                    \
      return cb->size;                                 \
    } else if (cb->tail > cb->head) {                  \
      return cb->tail - cb->head;                      \
    } else {                                           \
      return cb->size - (cb->head - cb->tail);         \
    }                                                  \
  }                                                    \
                                                       \
  static inline void name##_advance(name##* cb)        \
  {                                                    \
    name##_assert(cb);                                 \
    if (cb->full) {                                    \
      cb->tail = (cb->tail + 1) % size;                \
    }                                                  \
    cb->head = (cb->head + 1) % size;                  \
    cb->full = (cb->head == cb->tail);                 \
  }                                                    \
                                                       \
  static inline void name##_push(name##* cb, type val) \
  {                                                    \
    name##_assert(cb);                                 \
    cb->data[cb->head] = val;                          \
    name##_advance(cb);                                \
  }                                                    \
                                                       \
  static inline int name##_pop(name##* cb, type* val)  \
  {                                                    \
    name##_assert(cb);                                 \
    if (name##_empty(cb)) {                            \
      return 0;                                        \
    }                                                  \
    *val = cb->data[cb->tail];                         \
    cb->tail = (cb->tail + 1) % size;                  \
    cb->full = 0;                                      \
    return 1;                                          \
  }
