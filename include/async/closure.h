#pragma once

#include "common/types.h"

typedef struct
{
  void (*func) (void *);
  void *context;
} closure;

static inline DEFINE_DBG_ASSERT (closure, closure, c)
{
  ASSERT (c);
  ASSERT (c->func);
}

void clsr_execute (closure *cl);
