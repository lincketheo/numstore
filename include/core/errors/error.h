#pragma once

#include "core/dev/assert.h"
#include "core/ds/strings.h" // TODO
#include "core/intf/types.h" // TODO

struct lalloc_s;

typedef enum
{
  SUCCESS = 0,
  ERR_IO = -1,             // Generic IO Error,
  ERR_CORRUPT = -2,        // Corrupt page / file
  ERR_ALREADY_EXISTS = -5, // A thing already exists
  ERR_DOESNT_EXIST = -6,   // Something doesn't exist
  ERR_NOMEM = -9,          // No memory available
  ERR_INVALID_ARGUMENT = -13,
  ERR_INVALID_TYPE = -14,
  ERR_TYPE_DESER = -15,
  ERR_SYNTAX = -16,
  ERR_PAGE_STACK_OVERFLOW = -17,
  ERR_ARITH = -18,
  ERR_UNEXPECTED = -19,

  ERR_FALLBACK = -100000,
} err_t;

const char *err_t_to_str (err_t e);

typedef struct
{
  err_t cause_code;
  char cause_msg[256];
  u32 cmlen;

  string evidence[10];    // Only built if alloc is not null
  u32 elen;               // Length of evidence
  struct lalloc_s *alloc; // Sweeps ERRNOMEM under the rug
} error;

DEFINE_DBG_ASSERT_I (error, ok_error, e)
{
  ASSERT (e);
  ASSERT (e->cause_code == SUCCESS);
}

DEFINE_DBG_ASSERT_I (error, bad_error, e)
{
  ASSERT (e);
  ASSERT (e->cause_code < SUCCESS);
}

#define err_t_wrap(expr, e)                                         \
  do                                                                \
    {                                                               \
      err_t __ret = (err_t)expr;                                    \
      /*i_log_trace ("%s: %s\n", #expr, err_t_to_str (__ret));*/    \
      if (__ret < SUCCESS)                                          \
        {                                                           \
          return error_trailf_dbg (e, "In function: %s", __func__); \
        }                                                           \
    }                                                               \
  while (0)

#define err_t_wrap_null(expr, e)                                   \
  do                                                               \
    {                                                              \
      err_t __ret = (err_t)expr;                                   \
      /*i_log_trace ("%s: %s\n", #expr, err_t_to_str (__ret));*/   \
      if (__ret < SUCCESS)                                         \
        {                                                          \
          (void)error_trailf_dbg (e, "In function: %s", __func__); \
          return NULL;                                             \
        }                                                          \
    }                                                              \
  while (0)

#define err_t_wrap_break(expr, e)                                  \
  do                                                               \
    {                                                              \
      err_t __ret = (err_t)expr;                                   \
      /*i_log_trace ("%s: %s\n", #expr, err_t_to_str (__ret));*/   \
      if (__ret < SUCCESS)                                         \
        {                                                          \
          (void)error_trailf_dbg (e, "In function: %s", __func__); \
          break;                                                   \
        }                                                          \
    }                                                              \
  while (0)

#define err_t_log_swallow(expr, ename)   \
  do                                     \
    {                                    \
      error ename = error_create (NULL); \
      if (expr)                          \
        {                                \
          error_log_consume (&ename);    \
        }                                \
    }                                    \
  while (0)

#define err_t_continue(expr, ename)           \
  do                                          \
    {                                         \
      /* If already error, log swallow */     \
      if (err_t_from (ename))                 \
        {                                     \
          error _ename = error_create (NULL); \
          if (expr)                           \
            {                                 \
              error_log_consume (&_ename);    \
            }                                 \
        }                                     \
      /* Otherwise, run normally */           \
      else                                    \
        {                                     \
          expr;                               \
        }                                     \
    }                                         \
  while (0)

error error_create (struct lalloc_s *alloc);

#define err_t_from(eptr) (eptr)->cause_code

err_t error_causef (
    error *e,
    err_t c,
    const char *fmt, ...)
    __attribute__ ((format (printf, 3, 4)));

err_t error_trailf (
    error *e,
    const char *fmt, ...)
    __attribute__ ((format (printf, 2, 3)));

err_t error_change_causef (
    error *e,
    err_t c,
    const char *fmt, ...)
    __attribute__ ((format (printf, 3, 4)));

void error_log_consume (error *e);

bool error_equal (const error *left, const error *right);

#ifndef NDEBUG
#define error_trailf_dbg(e, ...) error_trailf (e, "DEBUG " __VA_ARGS__)
#else
#define error_trailf_dbg(e, ...) ((e)->cause_code)
#endif
