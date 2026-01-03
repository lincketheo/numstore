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
 *   TODO: Add description for spx_latch.c
 */

#include <numstore/core/spx_latch.h>

#include <numstore/core/assert.h>
#include <numstore/intf/os.h>
#include <numstore/test/testing.h>

#include <sched.h>
#include <stdatomic.h>
#include <stdint.h>
#include <unistd.h>

#define PENDING_BIT (1ULL << 31)
#define COUNT_MASK (~PENDING_BIT)

#define is_X_set(val) ((val & COUNT_MASK) == COUNT_MASK)
#define no_S_left(val) ((val & COUNT_MASK) == 0)
#define is_P_set(val) (val & PENDING_BIT)
#define X_lock (PENDING_BIT | COUNT_MASK)
#define P_lock(val) ((val) | PENDING_BIT)
#define S_lock(val) ((val) + 1)

#ifndef NDEBUG
// Check if running under Valgrind
#ifdef RUNNING_ON_VALGRIND
#define DEADLOCK_THRESH 100000
#else
#define DEADLOCK_THRESH 1000000000
#endif
#define YIELD_INTERVAL 1000
#endif

void
spx_latch_init (struct spx_latch *latch)
{
  atomic_init (&latch->state, 0);
}

void
spx_latch_lock_s (struct spx_latch *latch)
{
  uint_fast32_t expected;
#ifndef NDEBUG
  u32 nloops = 0;
#endif

  while (1)
    {
#ifndef NDEBUG
      if (nloops++ > DEADLOCK_THRESH)
        {
          ASSERTF (0, "Deadlock detected in s lock!\n");
        }
      // Yield CPU periodically to avoid busy-wait under valgrind
      if (nloops % YIELD_INTERVAL == 0)
        {
          sched_yield ();
        }
#endif
      expected = atomic_load (&latch->state);

      // Block on P or X lock
      if (is_P_set (expected) || is_X_set (expected))
        {
          continue;
        }

      // Verify nothing changed
      if (atomic_compare_exchange_weak (&latch->state, &expected, S_lock (expected)))
        {
          return;
        }
    }
}

void
spx_latch_unlock_s (struct spx_latch *latch)
{
  atomic_fetch_sub (&latch->state, 1);
}

void
spx_latch_lock_x (struct spx_latch *latch)
{
  uint_fast32_t expected;
#ifndef NDEBUG
  u32 nploops = 0;
  u32 nxloops = 0;
#endif

  while (1)
    {
#ifndef NDEBUG
      if (nploops++ > DEADLOCK_THRESH)
        {
          ASSERTF (0, "Deadlock detected in p lock!\n");
        }
      // Yield CPU periodically to avoid busy-wait under valgrind
      if (nploops % YIELD_INTERVAL == 0)
        {
          sched_yield ();
        }
#endif
      expected = atomic_load (&latch->state);

      // Pending lock is already set - skip it
      if (is_P_set (expected))
        {
          continue;
        }

      // Verify nothing changed and set P Lock
      if (!atomic_compare_exchange_weak (&latch->state, &expected, P_lock (expected)))
        {
          continue;
        }

      // drain S locks
      while (1)
        {
#ifndef NDEBUG
          if (nxloops++ > DEADLOCK_THRESH)
            {
              ASSERTF (0, "Deadlock detected in p lock!\n");
            }
          // Yield CPU periodically to avoid busy-wait under valgrind
          if (nxloops % YIELD_INTERVAL == 0)
            {
              sched_yield ();
            }
#endif
          expected = atomic_load (&latch->state);

          ASSERT (is_P_set (expected));

          // If number of S locks == 0
          if (no_S_left (expected))
            {
              // Verify nothing changed and set X Lock
              if (atomic_compare_exchange_weak (&latch->state, &expected, X_lock))
                {
                  return;
                }
            }
        }
    }
}

void
spx_latch_unlock_x (struct spx_latch *latch)
{
  atomic_store (&latch->state, 0);
}

void
spx_latch_upgrade_s_x (struct spx_latch *latch)
{
  uint_fast32_t expected;
#ifndef NDEBUG
  u32 nloops = 0;
#endif

  while (1)
    {
#ifndef NDEBUG
      if (nloops++ > DEADLOCK_THRESH)
        {
          ASSERTF (0, "Deadlock detected in upgrade!\n");
        }
      if (nloops % YIELD_INTERVAL == 0)
        {
          sched_yield ();
        }
#endif
      expected = atomic_load (&latch->state);

      // Must have at least one S lock (ours)
      ASSERT ((expected & COUNT_MASK) >= 1);

      // Can't upgrade if pending is already set
      if (is_P_set (expected))
        {
          continue;
        }

      if (!atomic_compare_exchange_weak (&latch->state, &expected, P_lock (expected - 1)))
        {
          continue;
        }

      // Same as X lock
      while (1)
        {
#ifndef NDEBUG
          if (nloops++ > DEADLOCK_THRESH)
            {
              ASSERTF (0, "Deadlock detected in upgrade drain!\n");
            }
          if (nloops % YIELD_INTERVAL == 0)
            {
              sched_yield ();
            }
#endif
          expected = atomic_load (&latch->state);

          ASSERT (is_P_set (expected));

          if (no_S_left (expected))
            {
              // All S locks drained, acquire X
              if (atomic_compare_exchange_weak (&latch->state, &expected, X_lock))
                {
                  return;
                }
            }
        }
    }
}

void
spx_latch_downgrade_x_s (struct spx_latch *latch)
{
  // Atomically go from X to S(1)
  // X lock is: PENDING_BIT | COUNT_MASK
  // S lock is: 1
  atomic_store (&latch->state, 1);
}

#ifndef NTEST

#include <semaphore.h>

// Thread functions
static struct spx_latch test_latch;
static sem_t sem1, sem2;
static atomic_int shared_counter;
static atomic_int state;

////////////////////////////////////////////////////////
/// X S Compatability
static void *
thread_x_lock_unlock (void *arg)
{
  spx_latch_lock_x (&test_latch);
  sem_post (&sem1);
  sem_wait (&sem2);
  spx_latch_unlock_x (&test_latch);
  return NULL;
}

static void *
thread_s_lock_after_x (void *arg)
{
  sem_wait (&sem1);
  spx_latch_lock_s (&test_latch);
  sem_post (&sem2);
  spx_latch_unlock_s (&test_latch);
  return NULL;
}

TEST_disabled (TT_UNIT, spx_latch_x_blocks_s)
{
  spx_latch_init (&test_latch);
  sem_init (&sem1, 0, 0);
  sem_init (&sem2, 0, 0);

  pthread_t t1, t2;
  pthread_create (&t1, NULL, thread_x_lock_unlock, NULL);
  pthread_create (&t2, NULL, thread_s_lock_after_x, NULL);

  pthread_join (t1, NULL);
  pthread_join (t2, NULL);

  sem_destroy (&sem1);
  sem_destroy (&sem2);
}

////////////////////////////////////////////////////////
/// X S Compatability

static pthread_barrier_t reader_barrier;
static atomic_int readers_acquired;

static void *
thread_multiple_readers (void *arg)
{
  pthread_barrier_wait (&reader_barrier);
  spx_latch_lock_s (&test_latch);
  atomic_fetch_add (&readers_acquired, 1);
  pthread_barrier_wait (&reader_barrier);
  spx_latch_unlock_s (&test_latch);
  return NULL;
}

TEST_disabled (TT_UNIT, spx_latch_multiple_s_readers)
{
  spx_latch_init (&test_latch);
  atomic_store (&readers_acquired, 0);

  int n_readers = 5;
  pthread_barrier_init (&reader_barrier, NULL, n_readers + 1);

  pthread_t threads[5];
  for (int i = 0; i < n_readers; i++)
    {
      pthread_create (&threads[i], NULL, thread_multiple_readers, NULL);
    }

  pthread_barrier_wait (&reader_barrier); // Wait for all to acquire
  test_assert_int_equal (atomic_load (&readers_acquired), n_readers);
  pthread_barrier_wait (&reader_barrier); // Let them unlock

  for (int i = 0; i < n_readers; i++)
    {
      pthread_join (threads[i], NULL);
    }

  pthread_barrier_destroy (&reader_barrier);
}

////////////////////////////////////////////////////////
/// X S Compatability

static sem_t upgrade_locked, upgrade_blocked, upgrade_done;

static void *
thread_upgrade_s_to_x (void *arg)
{
  spx_latch_lock_s (&test_latch);
  sem_post (&upgrade_locked);
  sem_wait (&upgrade_blocked);
  spx_latch_upgrade_s_x (&test_latch);
  sem_post (&upgrade_done);
  spx_latch_unlock_x (&test_latch);
  return NULL;
}

static void *
thread_s_lock_release_for_upgrade (void *arg)
{
  sem_wait (&upgrade_locked);
  spx_latch_lock_s (&test_latch);
  sem_post (&upgrade_blocked);
  spx_latch_unlock_s (&test_latch);
  sem_wait (&upgrade_done);
  return NULL;
}

TEST_disabled (TT_UNIT, spx_latch_upgrade_s_x)
{
  spx_latch_init (&test_latch);
  sem_init (&upgrade_locked, 0, 0);
  sem_init (&upgrade_blocked, 0, 0);
  sem_init (&upgrade_done, 0, 0);

  pthread_t t1, t2;
  pthread_create (&t1, NULL, thread_upgrade_s_to_x, NULL);
  pthread_create (&t2, NULL, thread_s_lock_release_for_upgrade, NULL);

  pthread_join (t1, NULL);
  pthread_join (t2, NULL);

  sem_destroy (&upgrade_locked);
  sem_destroy (&upgrade_blocked);
  sem_destroy (&upgrade_done);
}

////////////////////////////////////////////////////////
/// X S Compatability

static sem_t downgrade_x_locked, downgrade_reader_blocked;

static void *
thread_downgrade_x_to_s (void *arg)
{
  spx_latch_lock_x (&test_latch);
  atomic_store (&state, 1);
  sem_post (&downgrade_x_locked);
  sem_wait (&downgrade_reader_blocked);
  spx_latch_downgrade_x_s (&test_latch);
  atomic_store (&state, 2);
  while (atomic_load (&shared_counter) == 0)
    ; // Wait for reader
  spx_latch_unlock_s (&test_latch);
  return NULL;
}

static void *
thread_s_lock_after_downgrade (void *arg)
{
  sem_wait (&downgrade_x_locked);
  spx_latch_lock_s (&test_latch);
  sem_post (&downgrade_reader_blocked);
  atomic_load (&state);
  atomic_store (&shared_counter, 1);
  spx_latch_unlock_s (&test_latch);
  return NULL;
}

TEST_disabled (TT_UNIT, spx_latch_downgrade_x_s)
{
  spx_latch_init (&test_latch);
  atomic_store (&state, 0);
  atomic_store (&shared_counter, 0);
  sem_init (&downgrade_x_locked, 0, 0);
  sem_init (&downgrade_reader_blocked, 0, 0);

  pthread_t t1, t2;
  pthread_create (&t1, NULL, thread_downgrade_x_to_s, NULL);
  pthread_create (&t2, NULL, thread_s_lock_after_downgrade, NULL);

  pthread_join (t1, NULL);
  pthread_join (t2, NULL);

  sem_destroy (&downgrade_x_locked);
  sem_destroy (&downgrade_reader_blocked);
}

////////////////////////////////////////////////////////
/// X S Compatability

static void *
thread_x_writer_increments (void *arg)
{
  for (int i = 0; i < 100; i++)
    {
      spx_latch_lock_x (&test_latch);
      atomic_fetch_add (&shared_counter, 1);
      spx_latch_unlock_x (&test_latch);
    }
  return NULL;
}

TEST_disabled (TT_UNIT, spx_latch_concurrent_writers)
{
  spx_latch_init (&test_latch);
  atomic_store (&shared_counter, 0);

  int n_writers = 10;
  pthread_t threads[10];

  for (int i = 0; i < n_writers; i++)
    pthread_create (&threads[i], NULL, thread_x_writer_increments, NULL);

  for (int i = 0; i < n_writers; i++)
    pthread_join (threads[i], NULL);

  test_assert_int_equal (atomic_load (&shared_counter), 1000);
}

////////////////////////////////////////////////////////
/// X S Compatability

static pthread_barrier_t upgrade_barrier;
static atomic_int upgrade_order;

static void *
thread_concurrent_upgrade (void *arg)
{
  int *order = (int *)arg;

  spx_latch_lock_s (&test_latch);
  pthread_barrier_wait (&upgrade_barrier);
  spx_latch_upgrade_s_x (&test_latch);
  *order = atomic_fetch_add (&upgrade_order, 1);
  spx_latch_unlock_x (&test_latch);

  return NULL;
}

TEST_disabled (TT_UNIT, spx_latch_upgrade_serialization)
{
  spx_latch_init (&test_latch);
  atomic_store (&upgrade_order, 0);

  int n_threads = 3;
  pthread_barrier_init (&upgrade_barrier, NULL, n_threads);

  pthread_t threads[3];
  int orders[3];

  for (int i = 0; i < n_threads; i++)
    pthread_create (&threads[i], NULL, thread_concurrent_upgrade, &orders[i]);

  for (int i = 0; i < n_threads; i++)
    pthread_join (threads[i], NULL);

  // Verify all got unique order numbers
  int sum = 0;
  for (int i = 0; i < n_threads; i++)
    sum += orders[i];
  test_assert_int_equal (sum, 0 + 1 + 2); // 0+1+2 = 3

  pthread_barrier_destroy (&upgrade_barrier);
}

////////////////////////////////////////////////////////
/// X S Compatability

static void *
thread_writer_sets_state (void *arg)
{
  usleep (100);
  spx_latch_lock_x (&test_latch);
  atomic_store (&state, 1);
  usleep (1000);
  atomic_store (&state, 2);
  spx_latch_unlock_x (&test_latch);
  return NULL;
}

static void *
thread_reader_waits_for_writer (void *arg)
{
  while (atomic_load (&state) != 1)
    ;
  spx_latch_lock_s (&test_latch);
  atomic_load (&state);
  spx_latch_unlock_s (&test_latch);
  return NULL;
}

TEST_disabled (TT_UNIT, spx_latch_writer_blocks_reader)
{
  spx_latch_init (&test_latch);
  atomic_store (&state, 0);

  pthread_t writer, reader;
  pthread_create (&writer, NULL, thread_writer_sets_state, NULL);
  pthread_create (&reader, NULL, thread_reader_waits_for_writer, NULL);

  pthread_join (writer, NULL);
  pthread_join (reader, NULL);
}

#endif
