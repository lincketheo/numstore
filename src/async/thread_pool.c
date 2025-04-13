#include "async/thread_pool.h"
#include "async/closure.h"
#include "dev/errors.h"
#include "os/io.h"
#include "os/mm.h"

// TODO - abstract this out
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef struct closure_node closure_node;
struct closure_node
{
  closure *cl;
  closure_node *next;
};

static inline DEFINE_DBG_ASSERT_I (closure_node, closure_node, c)
{
  ASSERT (c);
  if (c->next != NULL)
    {
      closure_node_assert (c->next);
    }
  ASSERT (c->cl);
}

struct thread_pool
{
  closure_node *head;
  pthread_mutex_t mutex;
  pthread_t *threads;
  u64 num_threads;
  int done;
};

typedef struct
{
  closure *task;
  int done;
} get_task_result;

int
tp_is_spinning (const thread_pool *w)
{
  return (w->num_threads > 0) && (w->threads != NULL);
}

int
tp_not_spinning (const thread_pool *w)
{
  return (w->num_threads == 0) && (w->threads == NULL);
}

#define pthread_mutex_lock_ret(w)                        \
  if (pthread_mutex_lock (w))                            \
    {                                                    \
      i_log_error ("Failed to lock mutex. Reason: %s\n", \
                   strerror (errno));                    \
      return 1;                                          \
    }

#define pthread_mutex_unlock_ret(w)                        \
  if (pthread_mutex_unlock (w))                            \
    {                                                      \
      i_log_error ("Failed to unlock mutex. Reason: %s\n", \
                   strerror (errno));                      \
      return ERR_IO;                                       \
    }

static closure_node *
closure_node_create (void (*func) (void *), void *context)
{
  ASSERT (func);

  closure_node *ret = NULL;
  closure *c = NULL;

  if ((ret = i_malloc (sizeof *ret)) == NULL)
    {
      i_log_warn ("closure_node malloc\n");
      goto FAILED;
    }
  if ((c = i_malloc (sizeof *c)) == NULL)
    {
      i_log_warn ("closure malloc\n");
      goto FAILED;
    }

  c->func = func;
  c->context = context;

  ret->next = NULL;
  ret->cl = c;

  closure_node_assert (ret);

  return ret;

FAILED:
  if (ret)
    free (ret);
  if (c)
    free (c);
  return NULL;
}

// To reduce the number of calls to
// lock / unlock, returns both done and next
// task in one go
static int
get_task (thread_pool *w, get_task_result *res)
{
  thread_pool_assert (w);
  ASSERT (res);

  int done;
  closure_node *c = NULL;
  res->task = NULL;

  pthread_mutex_lock_ret (&w->mutex);

  c = w->head;
  if (c != NULL)
    w->head = c->next;
  done = w->done;

  pthread_mutex_unlock_ret (&w->mutex);

  // Free the closure node wrapper
  if (c)
    {
      res->task = c->cl;
      free (c);
    }

  res->done = done;

  return SUCCESS;
}

int
add_task (thread_pool *w, void (*func) (void *), void *context)
{
  thread_pool_assert (w);
  ASSERT (func);

  closure_node *node = NULL;
  if ((node = closure_node_create (func, context)) == NULL)
    {
      i_log_warn ("Failed to create a closure node while adding task\n");
      return ERR_IO; // TODO - return the right thing
    }

  pthread_mutex_lock_ret (&w->mutex);

  node->next = w->head;
  w->head = node;

  pthread_mutex_unlock_ret (&w->mutex);

  return SUCCESS;
}

thread_pool *
tp_open ()
{
  thread_pool *w = NULL;

  if ((w = i_malloc (sizeof (thread_pool))) == NULL)
    {
      i_log_warn ("Failed to allocate memory for thread_pool\n");
      goto failed;
    }
  if (pthread_mutex_init (&w->mutex, NULL))
    {
      i_log_warn ("Failed to create mutex for thread_pool\n");
      goto failed;
    }

  w->head = NULL;
  w->threads = NULL;
  w->num_threads = 0;

  thread_pool_assert (w);

  return w;

failed:
  if (w)
    free (w);
  return NULL;
}

static int
join_all (pthread_t *t, u64 num)
{
  ASSERT (t != NULL || num == 0);
  int ret = SUCCESS;
  if (t)
    {
      for (u32 i = 0; i < num; ++i)
        {
          if (pthread_join (t[i], NULL))
            {
              ret = ERR_IO;
              i_log_warn ("Failed to join thread: %d. Reason: %s\n",
                          i, strerror (errno));
            }
        }
    }
  return ret;
}

static int
cancel_all (pthread_t *t, u64 num)
{
  ASSERT (t != NULL || num == 0);
  int ret = SUCCESS;
  if (t)
    {
      for (u32 i = 0; i < num; ++i)
        {
          if (pthread_cancel (t[i]))
            {
              ret = ERR_IO;
              i_log_warn ("Failed to cancel thread: %d. Reason: %s\n",
                          i, strerror (errno));
            }
        }
    }
  return ret;
}

static int
cancel_and_join_all (pthread_t *t, u64 num)
{
  int _ret;
  int ret = SUCCESS;
  if ((_ret = cancel_all (t, num)))
    {
      i_log_warn ("Failed to cancel all in cancel and join all\n");
      ret = _ret;
    }
  if ((_ret = join_all (t, num)))
    {
      i_log_warn ("Failed to join all in cancel and join all\n");
      ret = _ret;
    }
  return ret;
}

int
tp_close (thread_pool *w)
{
  thread_pool_assert (w);

  int ret = SUCCESS;
  int _ret;

  if (w)
    {
      if (w->threads)
        {
          if ((_ret = cancel_and_join_all (w->threads, w->num_threads)))
            {
              i_log_warn ("Failed to cancel and join all threads\n");
              ret = _ret;
            }
          i_free (w->threads);
          w->threads = NULL;
          w->num_threads = 0;
        }

      if (w->head)
        {
          get_task_result result;
          while (1)
            {
              if ((_ret = get_task (w, &result)))
                {
                  i_log_warn ("Failed to get task\n");
                  ret = _ret;
                }
              if (_ret == SUCCESS && result.task)
                {
                  free (result.task);
                }
              else
                {
                  break;
                }
            }
        }

      if ((_ret = pthread_mutex_destroy (&w->mutex)))
        {
          i_log_warn ("Failed to destroy mutex. Reason: %s\n",
                      strerror (errno));
          ret = ERR_IO;
        }
      i_free (w);
    }
  return ret;
}

static void *
worker_thread (void *cl)
{
  ASSERT (cl);
  thread_pool *w = cl;
  thread_pool_assert (w);

  get_task_result res = { 0 };

  while (1)
    {
      if (get_task (w, &res))
        {
          i_log_warn ("Failed to get next task inside "
                      "worker thread. Exiting early\n");
          return NULL;
        }

      // If there's thread_pool,
      // then execute the thread_pool regardless of
      // done.
      if (res.task != NULL)
        {
          clsr_execute (res.task);
          free (res.task);
        }
      else if (res.done)
        {
          // If there's no more thread_pool left __and__ done, finish
          return NULL;
        }
    }
}

static int
tp_common (thread_pool *w, u32 num_threads)
{
  ASSERT (num_threads >= 1);
  ASSERT (tp_not_spinning (w));

  w->num_threads = 0;
  if ((w->threads = i_calloc (num_threads, sizeof (pthread_t))) == NULL)
    {
      i_log_warn ("Failed to allocate memory for threads.\n");
      goto failed;
    }

  for (u32 i = 0; i < num_threads; ++i)
    {
      if (pthread_create (&w->threads[i], NULL, worker_thread, w))
        {
          i_log_error ("Failed to create thread: %d. Reason: %s",
                       i, strerror (errno));
          goto failed;
        }
      else
        {
          w->num_threads++;
        }
    }

  return SUCCESS;

failed:
  i_log_warn ("Failed to create worker threads\n");
  if (w->threads)
    {
      if (cancel_and_join_all (w->threads, w->num_threads))
        {
          i_log_warn ("Failed to cancel and join all threads\n");
          // TODO - what should I do here?
        }
      free (w->threads);
      w->threads = NULL;
    }
  w->num_threads = 0;

  return ERR_IO; // TODO - double check this is ok
}

int
tp_execute_until_done (thread_pool *w, u32 num_threads)
{
  thread_pool_assert (w);
  ASSERT (num_threads >= 1);

  int ret;
  if ((ret = tp_spin (w, num_threads)))
    {
      i_log_warn ("Failed to spin threads while "
                  "running execute until done.\n");
      return ret;
    }

  ASSERT (tp_is_spinning (w));

  if ((ret = tp_stop (w)))
    {
      i_log_error ("Failed to stop spinning threads while "
                   "running execute until done.\n");
      return ret;
    }

  ASSERT (tp_not_spinning (w));

  return SUCCESS;
}

int
tp_spin (thread_pool *w, u64 num_threads)
{
  thread_pool_assert (w);
  ASSERT (!tp_is_spinning (w));
  ASSERT (num_threads >= 1);

  w->done = 0;

  int ret;
  if ((ret = tp_common (w, num_threads)))
    {
      i_log_warn ("Failed to initialize worker threads");
      goto failed;
    }

  ASSERT (tp_is_spinning (w));
  return SUCCESS;

failed:
  ASSERT (tp_not_spinning (w));
  return ret;
}

int
tp_stop (thread_pool *w)
{
  thread_pool_assert (w);
  ASSERT (tp_not_spinning (w));

  pthread_mutex_lock_ret (&w->mutex);

  w->done = 1;

  pthread_mutex_unlock_ret (&w->mutex);

  ASSERT (tp_is_spinning (w));

  int ret = SUCCESS;
  if ((ret = join_all (w->threads, w->num_threads)))
    {
      i_log_error ("Failed to join all threads");
      goto theend;
    }

theend:
  if (w->threads)
    {
      free (w->threads);
      w->threads = NULL;
      w->num_threads = 0;
    }
  return ret;
}
