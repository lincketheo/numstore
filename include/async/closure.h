#pragma once

#include "dev/assert.h"

typedef struct
{
  void (*func) (void *);
  void *context;
} closure;

static inline DEFINE_DBG_ASSERT_I (closure, closure, c)
{
  ASSERT (c);
  ASSERT (c->func);
}

void clsr_execute (closure *cl);
