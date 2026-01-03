/*
 * Copyright 2025 Theo Lincke
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Description:
 *   TODO: Add description for gr_lock.c
 */

#include <numstore/core/gr_lock.h>

#include <numstore/intf/os.h>
#include <numstore/intf/stdlib.h>
#include <numstore/intf/types.h>
#include <numstore/test/testing.h>

#include <unistd.h>

static const bool compatible[LM_COUNT][LM_COUNT] = {
  //       IS     IX     S      SIX    X
  [LM_IS] = { true, true, true, true, false },
  [LM_IX] = { true, true, false, false, false },
  [LM_S] = { true, false, true, false, false },
  [LM_SIX] = { true, false, false, false, false },
  [LM_X] = { false, false, false, false, false },
};

static const char *mode_names[LM_COUNT] = { "IS", "IX", "S", "SIX", "X" };

err_t
gr_lock_init (struct gr_lock *l, error *e)
{
  err_t result = i_mutex_create (&l->mutex, e);
  if (result != SUCCESS)
    {
      return result;
    }

  i_memset (l->holder_counts, 0, sizeof (l->holder_counts));
  l->waiters = NULL;

  return SUCCESS;
}

void
gr_lock_destroy (struct gr_lock *l)
{
  i_mutex_free (&l->mutex);

  while (l->waiters)
    {
      struct gr_lock_waiter *w = l->waiters;
      l->waiters = w->next;
      i_cond_free (&w->cond);
    }
}

static bool
is_compatible (struct gr_lock *l, enum lock_mode mode)
{
  for (int i = 0; i < LM_COUNT; i++)
    {
      if (l->holder_counts[i] > 0 && !compatible[mode][i])
        {
          return false;
        }
    }
  return true;
}

static void
wake_waiters (struct gr_lock *l)
{
  for (struct gr_lock_waiter *w = l->waiters; w; w = w->next)
    {
      if (is_compatible (l, w->mode))
        {
          i_cond_signal (&w->cond);
        }
    }
}

err_t
gr_lock (struct gr_lock *l, struct gr_lock_waiter *waiter, error *e)
{
  i_mutex_lock (&l->mutex);

  while (!is_compatible (l, waiter->mode))
    {
      err_t result = i_cond_create (&waiter->cond, e);
      if (result != SUCCESS)
        {
          i_mutex_unlock (&l->mutex);
          return result;
        }

      waiter->next = l->waiters;
      l->waiters = waiter;

      i_cond_wait (&waiter->cond, &l->mutex);

      struct gr_lock_waiter **ptr = &l->waiters;
      while (*ptr)
        {
          if (*ptr == waiter)
            {
              *ptr = waiter->next;
              break;
            }
          ptr = &(*ptr)->next;
        }

      i_cond_free (&waiter->cond);
    }

  l->holder_counts[waiter->mode]++;
  i_mutex_unlock (&l->mutex);
  return SUCCESS;
}

bool
gr_trylock (struct gr_lock *l, enum lock_mode mode)
{
  ASSERT (l);

  if (!i_mutex_try_lock (&l->mutex))
    {
      return false;
    }

  if (!is_compatible (l, mode))
    {
      i_mutex_unlock (&l->mutex);
      return false;
    }

  l->holder_counts[mode]++;
  i_mutex_unlock (&l->mutex);

  return true;
}

bool
gr_unlock (struct gr_lock *l, enum lock_mode mode)
{
  // TODO
  i_mutex_lock (&l->mutex);

  bool is_last = false;

  if (l->holder_counts[mode] > 0)
    {
      l->holder_counts[mode]--;

      bool any_holders = false;
      for (int i = 0; i < LM_COUNT; i++)
        {
          if (l->holder_counts[i] > 0)
            {
              any_holders = true;
              break;
            }
        }

      is_last = !any_holders && (l->waiters == NULL);

      wake_waiters (l);
    }

  i_mutex_unlock (&l->mutex);

  return is_last;
}

const char *
gr_lock_mode_name (enum lock_mode mode)
{
  if (mode >= 0 && mode < LM_COUNT)
    {
      return mode_names[mode];
    }
  return "INVALID";
}

#ifndef NTEST

struct lock_test_ctx
{
  struct gr_lock *lock;
  volatile int t1_acquired;
  volatile int t2_blocked;
  volatile int t2_acquired;
  volatile int counter;
  enum lock_mode mode1;
  enum lock_mode mode2;
};

static void
busy_wait_short (void)
{
  for (volatile int i = 0; i < 1000000; ++i)
    {
    }
}

// Test basic lock/unlock
TEST (TT_UNIT, gr_lock_basic)
{
  struct gr_lock lock;
  error e = error_create ();

  test_err_t_wrap (gr_lock_init (&lock, &e), &e);

  // Acquire and release each mode
  for (int mode = 0; mode < LM_COUNT; mode++)
    {
      test_err_t_wrap (gr_lock (&lock, &(struct gr_lock_waiter){ .mode = mode }, &e), &e);
      test_assert_equal (lock.holder_counts[mode], 1);
      gr_unlock (&lock, mode);
      test_assert_equal (lock.holder_counts[mode], 0);
    }

  gr_lock_destroy (&lock);
}

// Test multiple holders of same mode
TEST (TT_UNIT, gr_lock_multiple_holders)
{
  struct gr_lock lock;
  error e = error_create ();

  test_err_t_wrap (gr_lock_init (&lock, &e), &e);

  // S locks can be held multiple times
  test_err_t_wrap (gr_lock (&lock, &(struct gr_lock_waiter){ .mode = LM_S }, &e), &e);
  test_err_t_wrap (gr_lock (&lock, &(struct gr_lock_waiter){ .mode = LM_S }, &e), &e);
  test_err_t_wrap (gr_lock (&lock, &(struct gr_lock_waiter){ .mode = LM_S }, &e), &e);
  test_assert_equal (lock.holder_counts[LM_S], 3);

  gr_unlock (&lock, LM_S);
  test_assert_equal (lock.holder_counts[LM_S], 2);
  gr_unlock (&lock, LM_S);
  test_assert_equal (lock.holder_counts[LM_S], 1);
  gr_unlock (&lock, LM_S);
  test_assert_equal (lock.holder_counts[LM_S], 0);

  gr_lock_destroy (&lock);
}

// Helper thread functions for compatibility tests
static void *
thread_acquire_wait_release (void *arg)
{
  struct lock_test_ctx *ctx = arg;
  error e = error_create ();
  err_t result = gr_lock (ctx->lock, &(struct gr_lock_waiter){ .mode = ctx->mode1 }, &e);
  if (result != SUCCESS)
    {
      return NULL;
    }
  ctx->t1_acquired = 1;
  usleep (100000); // Hold lock for 100ms
  gr_unlock (ctx->lock, ctx->mode1);
  return NULL;
}

static void *
thread_try_acquire (void *arg)
{
  struct lock_test_ctx *ctx = arg;
  error e = error_create ();
  usleep (20000); // Let t1 acquire first
  ctx->t2_blocked = 1;
  err_t result = gr_lock (ctx->lock, &(struct gr_lock_waiter){ .mode = ctx->mode2 }, &e);
  if (result != SUCCESS)
    {
      return NULL;
    }
  ctx->t2_acquired = 1;
  ctx->t2_blocked = 0;
  gr_unlock (ctx->lock, ctx->mode2);
  return NULL;
}

// Test IS + IS (compatible)
TEST (TT_UNIT, gr_lock_is_is_compatible)
{
  struct gr_lock lock;
  error e = error_create ();

  test_err_t_wrap (gr_lock_init (&lock, &e), &e);

  struct lock_test_ctx ctx = { .lock = &lock, .mode1 = LM_IS, .mode2 = LM_IS };
  i_thread t1, t2;

  test_assert_equal (i_thread_create (&t1, thread_acquire_wait_release, &ctx, &e), SUCCESS);
  test_assert_equal (i_thread_create (&t2, thread_try_acquire, &ctx, &e), SUCCESS);

  usleep (50000); // Check if both acquired

  test_err_t_wrap (i_thread_join (&t1, &e), &e);
  test_err_t_wrap (i_thread_join (&t2, &e), &e);

  test_assert (ctx.t1_acquired);
  test_assert (ctx.t2_acquired);

  gr_lock_destroy (&lock);
}

// Test IS + IX (compatible)
TEST (TT_UNIT, gr_lock_is_ix_compatible)
{
  struct gr_lock lock;
  error e = error_create ();

  test_err_t_wrap (gr_lock_init (&lock, &e), &e);

  struct lock_test_ctx ctx = { .lock = &lock, .mode1 = LM_IS, .mode2 = LM_IX };
  i_thread t1, t2;
  test_assert_equal (i_thread_create (&t1, thread_acquire_wait_release, &ctx, &e), SUCCESS);
  test_assert_equal (i_thread_create (&t2, thread_try_acquire, &ctx, &e), SUCCESS);

  usleep (50000);

  test_err_t_wrap (i_thread_join (&t1, &e), &e);
  test_err_t_wrap (i_thread_join (&t2, &e), &e);

  test_assert (ctx.t1_acquired);
  test_assert (ctx.t2_acquired);

  gr_lock_destroy (&lock);
}

// Test IS + S (compatible)
TEST (TT_UNIT, gr_lock_is_s_compatible)
{
  struct gr_lock lock;
  error e = error_create ();

  test_err_t_wrap (gr_lock_init (&lock, &e), &e);

  struct lock_test_ctx ctx = { .lock = &lock, .mode1 = LM_IS, .mode2 = LM_S };
  i_thread t1, t2;
  test_assert_equal (i_thread_create (&t1, thread_acquire_wait_release, &ctx, &e), SUCCESS);
  test_assert_equal (i_thread_create (&t2, thread_try_acquire, &ctx, &e), SUCCESS);

  usleep (50000);

  test_err_t_wrap (i_thread_join (&t1, &e), &e);
  test_err_t_wrap (i_thread_join (&t2, &e), &e);

  test_assert (ctx.t1_acquired);
  test_assert (ctx.t2_acquired);

  gr_lock_destroy (&lock);
}

// Test IS + SIX (compatible)
TEST (TT_UNIT, gr_lock_is_six_compatible)
{
  struct gr_lock lock;
  error e = error_create ();

  test_err_t_wrap (gr_lock_init (&lock, &e), &e);

  struct lock_test_ctx ctx = { .lock = &lock, .mode1 = LM_IS, .mode2 = LM_SIX };
  i_thread t1, t2;
  test_assert_equal (i_thread_create (&t1, thread_acquire_wait_release, &ctx, &e), SUCCESS);
  test_assert_equal (i_thread_create (&t2, thread_try_acquire, &ctx, &e), SUCCESS);

  usleep (50000);

  test_err_t_wrap (i_thread_join (&t1, &e), &e);
  test_err_t_wrap (i_thread_join (&t2, &e), &e);

  test_assert (ctx.t1_acquired);
  test_assert (ctx.t2_acquired);

  gr_lock_destroy (&lock);
}

// Test IS + X (blocks)
TEST (TT_UNIT, gr_lock_is_x_blocks)
{
  struct gr_lock lock;
  error e = error_create ();

  test_err_t_wrap (gr_lock_init (&lock, &e), &e);

  struct lock_test_ctx ctx = { .lock = &lock, .mode1 = LM_IS, .mode2 = LM_X };
  i_thread t1, t2;
  test_assert_equal (i_thread_create (&t1, thread_acquire_wait_release, &ctx, &e), SUCCESS);
  test_assert_equal (i_thread_create (&t2, thread_try_acquire, &ctx, &e), SUCCESS);

  usleep (50000);
  // Sample state while threads are still running
  int t1_acquired_sample = ctx.t1_acquired;
  int t2_blocked_sample = ctx.t2_blocked;
  int t2_acquired_sample = ctx.t2_acquired;

  test_err_t_wrap (i_thread_join (&t1, &e), &e);
  test_err_t_wrap (i_thread_join (&t2, &e), &e);

  test_assert (t1_acquired_sample);
  test_assert (t2_blocked_sample);
  test_assert (!t2_acquired_sample);

  gr_lock_destroy (&lock);
}

// Test IX + IX (compatible)
TEST (TT_UNIT, gr_lock_ix_ix_compatible)
{
  struct gr_lock lock;
  error e = error_create ();

  test_err_t_wrap (gr_lock_init (&lock, &e), &e);

  struct lock_test_ctx ctx = { .lock = &lock, .mode1 = LM_IX, .mode2 = LM_IX };
  i_thread t1, t2;
  test_assert_equal (i_thread_create (&t1, thread_acquire_wait_release, &ctx, &e), SUCCESS);
  test_assert_equal (i_thread_create (&t2, thread_try_acquire, &ctx, &e), SUCCESS);

  usleep (50000);

  test_err_t_wrap (i_thread_join (&t1, &e), &e);
  test_err_t_wrap (i_thread_join (&t2, &e), &e);

  test_assert (ctx.t1_acquired);
  test_assert (ctx.t2_acquired);

  gr_lock_destroy (&lock);
}

// Test IX + S (blocks)
TEST (TT_UNIT, gr_lock_ix_s_blocks)
{
  struct gr_lock lock;
  error e = error_create ();

  test_err_t_wrap (gr_lock_init (&lock, &e), &e);

  struct lock_test_ctx ctx = { .lock = &lock, .mode1 = LM_IX, .mode2 = LM_S };
  i_thread t1, t2;
  test_assert_equal (i_thread_create (&t1, thread_acquire_wait_release, &ctx, &e), SUCCESS);
  test_assert_equal (i_thread_create (&t2, thread_try_acquire, &ctx, &e), SUCCESS);

  usleep (50000);
  // Sample state while threads are still running
  int t1_acquired_sample = ctx.t1_acquired;
  int t2_blocked_sample = ctx.t2_blocked;

  test_err_t_wrap (i_thread_join (&t1, &e), &e);
  test_err_t_wrap (i_thread_join (&t2, &e), &e);

  test_assert (t1_acquired_sample);
  test_assert (t2_blocked_sample);

  gr_lock_destroy (&lock);
}

// Test S + S (compatible)
TEST (TT_UNIT, gr_lock_s_s_compatible)
{
  struct gr_lock lock;
  error e = error_create ();

  test_err_t_wrap (gr_lock_init (&lock, &e), &e);

  struct lock_test_ctx ctx = { .lock = &lock, .mode1 = LM_S, .mode2 = LM_S };
  i_thread t1, t2;
  test_assert_equal (i_thread_create (&t1, thread_acquire_wait_release, &ctx, &e), SUCCESS);
  test_assert_equal (i_thread_create (&t2, thread_try_acquire, &ctx, &e), SUCCESS);

  usleep (50000);

  test_err_t_wrap (i_thread_join (&t1, &e), &e);
  test_err_t_wrap (i_thread_join (&t2, &e), &e);

  test_assert (ctx.t1_acquired);
  test_assert (ctx.t2_acquired);

  gr_lock_destroy (&lock);
}

// Test S + X (blocks)
TEST (TT_UNIT, gr_lock_s_x_blocks)
{
  struct gr_lock lock;
  error e = error_create ();

  test_err_t_wrap (gr_lock_init (&lock, &e), &e);

  struct lock_test_ctx ctx = { .lock = &lock, .mode1 = LM_S, .mode2 = LM_X };
  i_thread t1, t2;
  test_assert_equal (i_thread_create (&t1, thread_acquire_wait_release, &ctx, &e), SUCCESS);
  test_assert_equal (i_thread_create (&t2, thread_try_acquire, &ctx, &e), SUCCESS);

  usleep (50000);
  // Sample state while threads are still running
  int t1_acquired_sample = ctx.t1_acquired;
  int t2_blocked_sample = ctx.t2_blocked;

  test_err_t_wrap (i_thread_join (&t1, &e), &e);
  test_err_t_wrap (i_thread_join (&t2, &e), &e);

  test_assert (t1_acquired_sample);
  test_assert (t2_blocked_sample);

  gr_lock_destroy (&lock);
}

// Test SIX + IS (compatible - only compatible pair for SIX)
TEST (TT_UNIT, gr_lock_six_is_compatible)
{
  struct gr_lock lock;
  error e = error_create ();

  test_err_t_wrap (gr_lock_init (&lock, &e), &e);

  struct lock_test_ctx ctx = { .lock = &lock, .mode1 = LM_SIX, .mode2 = LM_IS };
  i_thread t1, t2;
  test_assert_equal (i_thread_create (&t1, thread_acquire_wait_release, &ctx, &e), SUCCESS);
  test_assert_equal (i_thread_create (&t2, thread_try_acquire, &ctx, &e), SUCCESS);

  usleep (50000);

  test_err_t_wrap (i_thread_join (&t1, &e), &e);
  test_err_t_wrap (i_thread_join (&t2, &e), &e);

  test_assert (ctx.t1_acquired);
  test_assert (ctx.t2_acquired);

  gr_lock_destroy (&lock);
}

// Test SIX + IX (blocks)
TEST (TT_UNIT, gr_lock_six_ix_blocks)
{
  struct gr_lock lock;
  error e = error_create ();

  test_err_t_wrap (gr_lock_init (&lock, &e), &e);

  struct lock_test_ctx ctx = { .lock = &lock, .mode1 = LM_SIX, .mode2 = LM_IX };
  i_thread t1, t2;
  test_assert_equal (i_thread_create (&t1, thread_acquire_wait_release, &ctx, &e), SUCCESS);
  test_assert_equal (i_thread_create (&t2, thread_try_acquire, &ctx, &e), SUCCESS);

  usleep (50000);
  // Sample state while threads are still running
  int t1_acquired_sample = ctx.t1_acquired;
  int t2_blocked_sample = ctx.t2_blocked;

  test_err_t_wrap (i_thread_join (&t1, &e), &e);
  test_err_t_wrap (i_thread_join (&t2, &e), &e);

  test_assert (t1_acquired_sample);
  test_assert (t2_blocked_sample);

  gr_lock_destroy (&lock);
}

// Test SIX + S (blocks)
TEST (TT_UNIT, gr_lock_six_s_blocks)
{
  struct gr_lock lock;
  error e = error_create ();

  test_err_t_wrap (gr_lock_init (&lock, &e), &e);

  struct lock_test_ctx ctx = { .lock = &lock, .mode1 = LM_SIX, .mode2 = LM_S };
  i_thread t1, t2;
  test_assert_equal (i_thread_create (&t1, thread_acquire_wait_release, &ctx, &e), SUCCESS);
  test_assert_equal (i_thread_create (&t2, thread_try_acquire, &ctx, &e), SUCCESS);

  usleep (50000);
  // Sample state while threads are still running
  int t1_acquired_sample = ctx.t1_acquired;
  int t2_blocked_sample = ctx.t2_blocked;

  test_err_t_wrap (i_thread_join (&t1, &e), &e);
  test_err_t_wrap (i_thread_join (&t2, &e), &e);

  test_assert (t1_acquired_sample);
  test_assert (t2_blocked_sample);

  gr_lock_destroy (&lock);
}

// Test X + X (blocks)
TEST (TT_UNIT, gr_lock_x_x_blocks)
{
  struct gr_lock lock;
  error e = error_create ();

  test_err_t_wrap (gr_lock_init (&lock, &e), &e);

  struct lock_test_ctx ctx = { .lock = &lock, .mode1 = LM_X, .mode2 = LM_X };
  i_thread t1, t2;
  test_assert_equal (i_thread_create (&t1, thread_acquire_wait_release, &ctx, &e), SUCCESS);
  test_assert_equal (i_thread_create (&t2, thread_try_acquire, &ctx, &e), SUCCESS);

  usleep (50000);
  // Sample state while threads are still running
  int t1_acquired_sample = ctx.t1_acquired;
  int t2_blocked_sample = ctx.t2_blocked;

  test_err_t_wrap (i_thread_join (&t1, &e), &e);
  test_err_t_wrap (i_thread_join (&t2, &e), &e);

  test_assert (t1_acquired_sample);
  test_assert (t2_blocked_sample);

  gr_lock_destroy (&lock);
}

// Test data race protection with exclusive locks
static void *
increment_thread (void *arg)
{
  struct lock_test_ctx *ctx = arg;
  error e = error_create ();
  for (int i = 0; i < 100; i++)
    {
      err_t result = gr_lock (ctx->lock, &(struct gr_lock_waiter){ .mode = LM_X }, &e);
      if (result != SUCCESS)
        {
          return NULL;
        }
      int old = ctx->counter;
      busy_wait_short ();
      ctx->counter = old + 1;
      gr_unlock (ctx->lock, LM_X);
    }
  return NULL;
}

TEST (TT_HEAVY, gr_lock_data_race_protection)
{
  struct gr_lock lock;
  error e = error_create ();

  test_err_t_wrap (gr_lock_init (&lock, &e), &e);

  struct lock_test_ctx ctx = { .lock = &lock };
  i_thread threads[5];
  for (int i = 0; i < 5; i++)
    {
      test_assert_equal (i_thread_create (&threads[i], increment_thread, &ctx, &e), SUCCESS);
    }

  for (int i = 0; i < 5; i++)
    {
      test_err_t_wrap (i_thread_join (&threads[i], &e), &e);
    }

  test_assert_equal (ctx.counter, 500); // 5 threads * 100 increments

  gr_lock_destroy (&lock);
}

//==============================================================================
// COMPREHENSIVE STRESS TESTS
//==============================================================================

/**
 * Thread diagram for waiter queue stress test:
 *
 * This test creates a complex waiter queue scenario:
 *
 * Main: Holds X lock
 * T1-T3: Try to acquire S locks (blocked by X, added to waiter queue)
 * T4-T5: Try to acquire X locks (blocked by X, added to waiter queue)
 * T6-T7: Try to acquire IS locks (blocked by X, added to waiter queue)
 * Main: Releases X -> All compatible locks should wake up
 *
 * This tests:
 * - Waiter queue management with many waiters
 * - Correct wake-up logic (only wake compatible locks)
 * - No lost wakeups
 */
struct waiter_queue_ctx
{
  struct gr_lock *lock;
  volatile int s_acquired[10];
  volatile int x_acquired[10];
  volatile int is_acquired[10];
  volatile int main_released;
};

static void *
waiter_s_thread (void *arg)
{
  struct waiter_queue_ctx *ctx = arg;
  error e = error_create ();

  usleep (2000); // Let main thread acquire X

  err_t result = gr_lock (ctx->lock, &(struct gr_lock_waiter){ .mode = LM_S }, &e);
  if (result == SUCCESS)
    {
      for (int i = 0; i < 10; i++)
        {
          if (__sync_bool_compare_and_swap (&ctx->s_acquired[i], 0, 1))
            {
              break;
            }
        }
      gr_unlock (ctx->lock, LM_S);
    }
  return NULL;
}

static void *
waiter_x_thread (void *arg)
{
  struct waiter_queue_ctx *ctx = arg;
  error e = error_create ();

  usleep (2000); // Let main thread acquire X

  err_t result = gr_lock (ctx->lock, &(struct gr_lock_waiter){ .mode = LM_X }, &e);
  if (result == SUCCESS)
    {
      for (int i = 0; i < 10; i++)
        {
          if (__sync_bool_compare_and_swap (&ctx->x_acquired[i], 0, 1))
            {
              break;
            }
        }
      gr_unlock (ctx->lock, LM_X);
    }
  return NULL;
}

static void *
waiter_is_thread (void *arg)
{
  struct waiter_queue_ctx *ctx = arg;
  error e = error_create ();

  usleep (2000); // Let main thread acquire X

  err_t result = gr_lock (ctx->lock, &(struct gr_lock_waiter){ .mode = LM_IS }, &e);
  if (result == SUCCESS)
    {
      for (int i = 0; i < 10; i++)
        {
          if (__sync_bool_compare_and_swap (&ctx->is_acquired[i], 0, 1))
            {
              break;
            }
        }
      gr_unlock (ctx->lock, LM_IS);
    }
  return NULL;
}

TEST (TT_HEAVY, gr_lock_waiter_queue_stress)
{
  struct gr_lock lock;
  error e = error_create ();

  test_err_t_wrap (gr_lock_init (&lock, &e), &e);

  struct waiter_queue_ctx ctx = { .lock = &lock };
  i_thread threads[12];

  // Main thread acquires X
  test_err_t_wrap (gr_lock (&lock, &(struct gr_lock_waiter){ .mode = LM_X }, &e), &e);

  // Start S lock threads
  for (int i = 0; i < 3; i++)
    {
      test_assert_equal (i_thread_create (&threads[i], waiter_s_thread, &ctx, &e), SUCCESS);
    }

  // Start X lock threads
  for (int i = 3; i < 5; i++)
    {
      test_assert_equal (i_thread_create (&threads[i], waiter_x_thread, &ctx, &e), SUCCESS);
    }

  // Start IS lock threads
  for (int i = 5; i < 8; i++)
    {
      test_assert_equal (i_thread_create (&threads[i], waiter_is_thread, &ctx, &e), SUCCESS);
    }

  // Let all threads queue up
  usleep (10000);

  // Release X - this should wake up compatible waiters (IS, S can run together)
  ctx.main_released = 1;
  gr_unlock (&lock, LM_X);

  // Wait for all threads
  for (int i = 0; i < 8; i++)
    {
      test_err_t_wrap (i_thread_join (&threads[i], &e), &e);
    }

  // All threads should have acquired eventually
  int s_count = 0, x_count = 0, is_count = 0;
  for (int i = 0; i < 10; i++)
    {
      if (ctx.s_acquired[i])
        s_count++;
      if (ctx.x_acquired[i])
        x_count++;
      if (ctx.is_acquired[i])
        is_count++;
    }

  test_assert (s_count >= 2); // At least 2 S locks should have acquired
  test_assert (x_count >= 1); // At least 1 X lock should have acquired
  test_assert (is_count >= 2); // At least 2 IS locks should have acquired

  gr_lock_destroy (&lock);
}

/**
 * Thread diagram for mixed mode stress test:
 *
 * Multiple threads acquire different lock modes in a pattern that
 * stresses the compatibility matrix:
 *
 * T1: IS -> IX -> IS -> IX (repeat)
 * T2: S -> S -> S (multiple S locks)
 * T3: IX -> X -> IX -> X
 * T4: IS -> S -> IS -> S
 * T5: X -> X -> X (serialize access)
 *
 * This tests:
 * - All compatibility matrix entries under load
 * - Lock holder count tracking
 * - Correct serialization and parallelization
 */
struct mixed_mode_ctx
{
  struct gr_lock *lock;
  volatile int counter;
  volatile int operations[10];
};

static void *
mixed_mode_thread1 (void *arg)
{
  struct mixed_mode_ctx *ctx = arg;
  error e = error_create ();

  for (int i = 0; i < 10; i++)
    {
      err_t result = gr_lock (ctx->lock, &(struct gr_lock_waiter){ .mode = LM_IS }, &e);
      if (result == SUCCESS)
        {
          busy_wait_short ();
          gr_unlock (ctx->lock, LM_IS);
          ctx->operations[0]++;
        }

      result = gr_lock (ctx->lock, &(struct gr_lock_waiter){ .mode = LM_IX }, &e);
      if (result == SUCCESS)
        {
          busy_wait_short ();
          gr_unlock (ctx->lock, LM_IX);
          ctx->operations[0]++;
        }
    }
  return NULL;
}

static void *
mixed_mode_thread2 (void *arg)
{
  struct mixed_mode_ctx *ctx = arg;
  error e = error_create ();

  for (int i = 0; i < 20; i++)
    {
      err_t result = gr_lock (ctx->lock, &(struct gr_lock_waiter){ .mode = LM_S }, &e);
      if (result == SUCCESS)
        {
          busy_wait_short ();
          gr_unlock (ctx->lock, LM_S);
          ctx->operations[1]++;
        }
    }
  return NULL;
}

static void *
mixed_mode_thread3 (void *arg)
{
  struct mixed_mode_ctx *ctx = arg;
  error e = error_create ();

  for (int i = 0; i < 10; i++)
    {
      err_t result = gr_lock (ctx->lock, &(struct gr_lock_waiter){ .mode = LM_IX }, &e);
      if (result == SUCCESS)
        {
          busy_wait_short ();
          gr_unlock (ctx->lock, LM_IX);
          ctx->operations[2]++;
        }

      result = gr_lock (ctx->lock, &(struct gr_lock_waiter){ .mode = LM_X }, &e);
      if (result == SUCCESS)
        {
          ctx->counter++;
          gr_unlock (ctx->lock, LM_X);
          ctx->operations[2]++;
        }
    }
  return NULL;
}

static void *
mixed_mode_thread4 (void *arg)
{
  struct mixed_mode_ctx *ctx = arg;
  error e = error_create ();

  for (int i = 0; i < 10; i++)
    {
      err_t result = gr_lock (ctx->lock, &(struct gr_lock_waiter){ .mode = LM_IS }, &e);
      if (result == SUCCESS)
        {
          busy_wait_short ();
          gr_unlock (ctx->lock, LM_IS);
          ctx->operations[3]++;
        }

      result = gr_lock (ctx->lock, &(struct gr_lock_waiter){ .mode = LM_S }, &e);
      if (result == SUCCESS)
        {
          busy_wait_short ();
          gr_unlock (ctx->lock, LM_S);
          ctx->operations[3]++;
        }
    }
  return NULL;
}

static void *
mixed_mode_thread5 (void *arg)
{
  struct mixed_mode_ctx *ctx = arg;
  error e = error_create ();

  for (int i = 0; i < 15; i++)
    {
      err_t result = gr_lock (ctx->lock, &(struct gr_lock_waiter){ .mode = LM_X }, &e);
      if (result == SUCCESS)
        {
          ctx->counter++;
          gr_unlock (ctx->lock, LM_X);
          ctx->operations[4]++;
        }
    }
  return NULL;
}

TEST (TT_HEAVY, gr_lock_mixed_mode_stress)
{
  struct gr_lock lock;
  error e = error_create ();

  test_err_t_wrap (gr_lock_init (&lock, &e), &e);

  struct mixed_mode_ctx ctx = { .lock = &lock };
  i_thread threads[5];

  test_assert_equal (i_thread_create (&threads[0], mixed_mode_thread1, &ctx, &e), SUCCESS);
  test_assert_equal (i_thread_create (&threads[1], mixed_mode_thread2, &ctx, &e), SUCCESS);
  test_assert_equal (i_thread_create (&threads[2], mixed_mode_thread3, &ctx, &e), SUCCESS);
  test_assert_equal (i_thread_create (&threads[3], mixed_mode_thread4, &ctx, &e), SUCCESS);
  test_assert_equal (i_thread_create (&threads[4], mixed_mode_thread5, &ctx, &e), SUCCESS);

  for (int i = 0; i < 5; i++)
    {
      test_err_t_wrap (i_thread_join (&threads[i], &e), &e);
    }

  // Verify all operations completed
  test_assert (ctx.operations[0] > 0);
  test_assert (ctx.operations[1] > 0);
  test_assert (ctx.operations[2] > 0);
  test_assert (ctx.operations[3] > 0);
  test_assert (ctx.operations[4] > 0);

  // Counter should be exactly: 10 (thread3) + 15 (thread5) = 25
  test_assert_equal (ctx.counter, 25);

  gr_lock_destroy (&lock);
}

/**
 * Thread diagram for trylock stress test:
 *
 * T1: Holds S lock for duration
 * T2-T5: Repeatedly trylock X (should fail), then acquire X when T1 releases
 *
 * This tests:
 * - gr_trylock returns false when incompatible lock is held
 * - gr_trylock returns true when compatible
 * - No deadlocks or races in trylock path
 */
struct trylock_ctx
{
  struct gr_lock *lock;
  volatile int trylock_successes;
  volatile int trylock_failures;
  volatile int t1_released;
};

static void *
trylock_holder_thread (void *arg)
{
  struct trylock_ctx *ctx = arg;
  error e = error_create ();

  // Hold S lock for a while
  if (gr_lock (ctx->lock, &(struct gr_lock_waiter){ .mode = LM_S }, &e) != SUCCESS)
    {
      return NULL;
    }
  usleep (20000);
  gr_unlock (ctx->lock, LM_S);
  ctx->t1_released = 1;

  return NULL;
}

static void *
trylock_thread (void *arg)
{
  struct trylock_ctx *ctx = arg;
  int successes = 0;
  int failures = 0;

  // Try to acquire X lock repeatedly
  for (int i = 0; i < 100; i++)
    {
      if (gr_trylock (ctx->lock, LM_X))
        {
          successes++;
          gr_unlock (ctx->lock, LM_X);
        }
      else
        {
          failures++;
        }
      sched_yield ();
    }

  __sync_fetch_and_add (&ctx->trylock_successes, successes);
  __sync_fetch_and_add (&ctx->trylock_failures, failures);

  return NULL;
}

TEST (TT_HEAVY, gr_lock_trylock_stress)
{
  struct gr_lock lock;
  error e = error_create ();

  test_err_t_wrap (gr_lock_init (&lock, &e), &e);

  struct trylock_ctx ctx = { .lock = &lock };
  i_thread holder;
  i_thread threads[4];

  test_assert_equal (i_thread_create (&holder, trylock_holder_thread, &ctx, &e), SUCCESS);

  for (int i = 0; i < 4; i++)
    {
      test_assert_equal (i_thread_create (&threads[i], trylock_thread, &ctx, &e), SUCCESS);
    }

  test_err_t_wrap (i_thread_join (&holder, &e), &e);

  for (int i = 0; i < 4; i++)
    {
      test_err_t_wrap (i_thread_join (&threads[i], &e), &e);
    }

  // Should have had both successes and failures
  test_assert (ctx.trylock_failures > 0);  // Failed while S lock was held
  test_assert (ctx.trylock_successes > 0); // Succeeded after S released

  gr_lock_destroy (&lock);
}

/**
 * Thread diagram for SIX lock stress test:
 *
 * SIX (Shared Intent Exclusive) is special:
 * - Compatible only with IS
 * - Incompatible with IX, S, SIX, X
 *
 * T1: Acquires SIX
 * T2-T3: Try IS (should succeed - compatible with SIX)
 * T4: Tries IX (should block)
 * T5: Tries S (should block)
 * T6: Tries X (should block)
 */
struct six_stress_ctx
{
  struct gr_lock *lock;
  volatile int six_acquired;
  volatile int is_acquired;
  volatile int ix_blocked;
  volatile int s_blocked;
  volatile int x_blocked;
  volatile int six_released;
};

static void *
six_holder_thread (void *arg)
{
  struct six_stress_ctx *ctx = arg;
  error e = error_create ();

  if (gr_lock (ctx->lock, &(struct gr_lock_waiter){ .mode = LM_SIX }, &e) != SUCCESS)
    {
      return NULL;
    }
  ctx->six_acquired = 1;

  usleep (20000); // Hold SIX for a while

  gr_unlock (ctx->lock, LM_SIX);
  ctx->six_released = 1;

  return NULL;
}

static void *
six_is_thread (void *arg)
{
  struct six_stress_ctx *ctx = arg;
  error e = error_create ();

  usleep (5000); // Let SIX acquire first

  if (gr_lock (ctx->lock, &(struct gr_lock_waiter){ .mode = LM_IS }, &e) != SUCCESS)
    {
      return NULL;
    }
  ctx->is_acquired = 1;
  gr_unlock (ctx->lock, LM_IS);

  return NULL;
}

static void *
six_ix_thread (void *arg)
{
  struct six_stress_ctx *ctx = arg;
  error e = error_create ();

  usleep (5000); // Let SIX acquire first
  ctx->ix_blocked = 1;

  if (gr_lock (ctx->lock, &(struct gr_lock_waiter){ .mode = LM_IX }, &e) != SUCCESS)
    {
      return NULL;
    }
  ctx->ix_blocked = 0; // Acquired, no longer blocked
  gr_unlock (ctx->lock, LM_IX);

  return NULL;
}

static void *
six_s_thread (void *arg)
{
  struct six_stress_ctx *ctx = arg;
  error e = error_create ();

  usleep (5000); // Let SIX acquire first
  ctx->s_blocked = 1;

  if (gr_lock (ctx->lock, &(struct gr_lock_waiter){ .mode = LM_S }, &e) != SUCCESS)
    {
      return NULL;
    }
  ctx->s_blocked = 0; // Acquired, no longer blocked
  gr_unlock (ctx->lock, LM_S);

  return NULL;
}

static void *
six_x_thread (void *arg)
{
  struct six_stress_ctx *ctx = arg;
  error e = error_create ();

  usleep (5000); // Let SIX acquire first
  ctx->x_blocked = 1;

  if (gr_lock (ctx->lock, &(struct gr_lock_waiter){ .mode = LM_X }, &e) != SUCCESS)
    {
      return NULL;
    }
  ctx->x_blocked = 0; // Acquired, no longer blocked
  gr_unlock (ctx->lock, LM_X);

  return NULL;
}

TEST (TT_HEAVY, gr_lock_six_mode_stress)
{
  struct gr_lock lock;
  error e = error_create ();

  test_err_t_wrap (gr_lock_init (&lock, &e), &e);

  struct six_stress_ctx ctx = { .lock = &lock };
  i_thread threads[6];

  test_assert_equal (i_thread_create (&threads[0], six_holder_thread, &ctx, &e), SUCCESS);
  test_assert_equal (i_thread_create (&threads[1], six_is_thread, &ctx, &e), SUCCESS);
  test_assert_equal (i_thread_create (&threads[2], six_is_thread, &ctx, &e), SUCCESS);
  test_assert_equal (i_thread_create (&threads[3], six_ix_thread, &ctx, &e), SUCCESS);
  test_assert_equal (i_thread_create (&threads[4], six_s_thread, &ctx, &e), SUCCESS);
  test_assert_equal (i_thread_create (&threads[5], six_x_thread, &ctx, &e), SUCCESS);

  // Sample state while SIX is held
  usleep (10000);
  int is_acquired_sample = ctx.is_acquired;
  int ix_blocked_sample = ctx.ix_blocked;
  int s_blocked_sample = ctx.s_blocked;
  int x_blocked_sample = ctx.x_blocked;

  for (int i = 0; i < 6; i++)
    {
      test_err_t_wrap (i_thread_join (&threads[i], &e), &e);
    }

  // IS should have acquired (compatible with SIX)
  test_assert (is_acquired_sample);

  // IX, S, X should have been blocked while SIX was held
  test_assert (ix_blocked_sample);
  test_assert (s_blocked_sample);
  test_assert (x_blocked_sample);

  gr_lock_destroy (&lock);
}

/**
 * High contention stress test - many threads hammering the lock
 *
 * This creates maximum contention to stress test:
 * - Lock/unlock performance under load
 * - Waiter queue under heavy churn
 * - No deadlocks or livelocks
 */
struct high_contention_ctx
{
  struct gr_lock *lock;
  volatile int counter;
};

static void *
high_contention_thread (void *arg)
{
  struct high_contention_ctx *ctx = arg;
  error e = error_create ();

  for (int i = 0; i < 50; i++)
    {
      enum lock_mode mode = (enum lock_mode) (i % LM_COUNT);

      if (gr_lock (ctx->lock, &(struct gr_lock_waiter){ .mode = mode }, &e) != SUCCESS)
        {
          continue;
        }

      if (mode == LM_X)
        {
          ctx->counter++;
        }

      gr_unlock (ctx->lock, mode);
    }

  return NULL;
}

TEST (TT_HEAVY, gr_lock_high_contention)
{
  struct gr_lock lock;
  error e = error_create ();

  test_err_t_wrap (gr_lock_init (&lock, &e), &e);

  struct high_contention_ctx ctx = { .lock = &lock };
  i_thread threads[10];

  for (int i = 0; i < 10; i++)
    {
      test_assert_equal (i_thread_create (&threads[i], high_contention_thread, &ctx, &e), SUCCESS);
    }

  for (int i = 0; i < 10; i++)
    {
      test_err_t_wrap (i_thread_join (&threads[i], &e), &e);
    }

  // Each thread increments counter 10 times (when it gets X lock, i % 5 == 4)
  test_assert_equal (ctx.counter, 100); // 10 threads * 10 X locks each

  gr_lock_destroy (&lock);
}

#endif
