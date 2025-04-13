#pragma once

#include "dev/assert.h"
#include "os/types.h"

typedef struct thread_pool thread_pool;

int tp_is_spinning (const thread_pool *w);

int tp_not_spinning (const thread_pool *w);

static inline DEFINE_DBG_ASSERT_I (thread_pool, thread_pool, t)
{
  ASSERT (t);
  ASSERT (tp_is_spinning (t) || tp_not_spinning (t));
}

thread_pool *tp_open ();

int tp_close (thread_pool *w);

int tp_add_task (thread_pool *w, void (*func) (void *), void *context);

// Execute all of the work queued on [w] using [num_threads] threads.
// When w runs out of tasks, it stops execution
int tp_execute_until_done (thread_pool *w, u32 num_threads);

// Spins a thread pool continuously until stop_thread_pool is
// called. In the meantime, you can add_task as much as you
// want (thread safe)
int tp_spin (thread_pool *w, u64 num_threads);

int tp_stop (thread_pool *w);
