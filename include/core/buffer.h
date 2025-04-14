#pragma once

#include "dev/assert.h"
#include "dev/errors.h"
#include "os/mm.h"
#include "os/types.h"

// TODO - define max size
#define DEFINE_DYN_BUFFER(type, name)                                    \
  typedef struct                                                         \
  {                                                                      \
    type *data;                                                          \
    u64 cap;                                                             \
    u64 len;                                                             \
  } name##_buffer;                                                       \
                                                                         \
  static inline DEFINE_DBG_ASSERT_I (name##_buffer, name##_buffer, b)    \
  {                                                                      \
    ASSERT (b);                                                          \
    ASSERT (b->data);                                                    \
    ASSERT (b->cap > 0);                                                 \
    ASSERT (b->len <= b->cap);                                           \
  }                                                                      \
                                                                         \
  static inline int name##_buffer_malloc (name##_buffer *dest, u64 cap)  \
  {                                                                      \
    ASSERT (dest);                                                       \
    ASSERT (cap > 0);                                                    \
    ASSERT (dest->data == NULL);                                         \
    ASSERT (dest->cap == 0);                                             \
    ASSERT (dest->len == 0);                                             \
                                                                         \
    type *ret = i_malloc (cap * sizeof *ret);                            \
    if (ret == NULL)                                                     \
      {                                                                  \
        i_log_warn ("Failed to allocate data for " #name " buffer\n");   \
        return ERR_IO;                                                   \
      }                                                                  \
                                                                         \
    dest->data = ret;                                                    \
    dest->cap = cap;                                                     \
    dest->len = 0;                                                       \
    return SUCCESS;                                                      \
  }                                                                      \
                                                                         \
  static inline void name##_buffer_free (name##_buffer *b)               \
  {                                                                      \
    ASSERT (b);                                                          \
    name##_buffer_assert (b);                                            \
    i_free (b->data);                                                    \
  }                                                                      \
                                                                         \
  static inline int name##_buffer_resize (name##_buffer *b, u64 cap)     \
  {                                                                      \
    name##_buffer_assert (b);                                            \
    ASSERT (cap > 0);                                                    \
    type *ret = i_realloc (b->data, cap);                                \
    if (ret == NULL)                                                     \
      {                                                                  \
        i_log_warn ("Failed to reallocate data for " #name " buffer\n"); \
        return ERR_IO;                                                   \
      }                                                                  \
                                                                         \
    b->data = ret;                                                       \
    b->cap = cap;                                                        \
                                                                         \
    return SUCCESS;                                                      \
  }
