#include "core/errors/error.h"

#include <stdarg.h> // TODO
#include <stddef.h> // TODO
#include <stdio.h>  // TODO

#include "core/dev/assert.h"   // DEFINE_DBG_ASSERT_I
#include "core/intf/logging.h" // i_log_error
#include "core/intf/stdlib.h"  // i_vsnprintf
#include "core/mm/lalloc.h"    // lalloc
#include "core/utils/macros.h" // TODO

DEFINE_DBG_ASSERT_I (error, error, e)
{
  ASSERT (e);
  if (e->cause_code != SUCCESS)
    {
      ASSERT (e->cmlen > 0); // All errors have a cause message
    }

  /**
   * Cause is required before evidence
   */
  if (e->cmlen == 0)
    {
      ASSERT (e->elen == 0);
    }

  ASSERT (e->elen <= 10);

  /**
   * Don't allow evidence on null allocator
   */
  if (e->alloc == NULL)
    {
      ASSERT (e->elen == 0);
    }
}

const char *
err_t_to_str (err_t e)
{
  switch (e)
    {
    case SUCCESS:
      return "SUCCESS";
    case ERR_IO:
      return "ERR_IO";
    case ERR_CORRUPT:
      return "ERR_CORRUPT";
    case ERR_ALREADY_EXISTS:
      return "ERR_ALREADY_EXISTS";
    case ERR_DOESNT_EXIST:
      return "ERR_DOESNT_EXIST";
    case ERR_NOMEM:
      return "ERR_NOMEM";
    case ERR_INVALID_ARGUMENT:
      return "ERR_INVALID_ARGUMENT";
    case ERR_INVALID_TYPE:
      return "ERR_INVALID_TYPE";
    case ERR_TYPE_DESER:
      return "ERR_TYPE_DESER";
    case ERR_SYNTAX:
      return "ERR_SYNTAX";
    case ERR_PAGE_STACK_OVERFLOW:
      return "ERR_PAGE_STACK_OVERFLOW";
    case ERR_ARITH:
      return "ERR_ARITH";
    case ERR_UNEXPECTED:
      return "ERR_UNEXPECTED";

    case ERR_FALLBACK:
      return "ERR_FALLBACK";
    }
  UNREACHABLE ();
}

error
error_create (lalloc *alloc)
{
  error ret = {
    .cause_code = SUCCESS,

    .cmlen = 0,

    .elen = 0,
    .alloc = alloc
  };

  error_assert (&ret);

  return ret;
}

err_t
error_causef (error *e, err_t c, const char *fmt, ...)
{
  error_assert (e);
  ASSERT (fmt);
  ASSERT (e->cause_code == SUCCESS); // Can only go from good -> cause
  e->cause_code = c;

  /**
   * Print into cause message
   * (255 because of null terminator)
   */
  va_list ap;
  va_start (ap, fmt);

  va_list ap_copy;
  va_copy (ap_copy, ap);
  u32 cmlen = i_vsnprintf (e->cause_msg, 256, fmt, ap_copy);
  e->cmlen = MIN (cmlen, 255);
  va_end (ap_copy);
  va_end (ap);

  i_log_error ("%.*s\n", e->cmlen, e->cause_msg);

  return err_t_from (e);
}

err_t
error_trailf (error *e, const char *fmt, ...)
{
  error_assert (e);
  ASSERT (fmt);
  ASSERT (e->cause_code != SUCCESS); // Only trail on existing errors

  if (e->alloc == NULL)
    {
      return err_t_from (e);
    }

  if (10 == e->elen)
    {
      return err_t_from (e);
    }

  va_list ap = { 0 };
  va_start (ap, fmt);
  u32 n = i_vsnprintf (NULL, 0, fmt, ap) + 1;
  va_end (ap);

  void *buf = lmalloc (e->alloc, n, 1);

  if (buf == NULL)
    {
      return err_t_from (e);
    }

  return err_t_from (e);
}

err_t
error_change_causef (
    error *e,
    err_t c,
    const char *fmt, ...)
{
  va_list ap;
  va_start (ap, fmt);
  err_t prev = error_trailf (e, fmt, ap);
  ASSERT (prev != c);
  va_end (ap);
  e->cause_code = c;
  return err_t_from (e);
}

void
error_log_consume (error *e)
{
  error_assert (e);
  ASSERT (e->cause_code != SUCCESS);

  i_log_error ("%.*s\n", e->cmlen, e->cause_msg);
  if (e->alloc)
    {
      lalloc_reset (e->alloc);
    }

  for (u32 i = 0; i < e->elen; ++i)
    {
      string msg = e->evidence[i];
      i_log_warn ("TRACEBACK: %.*s\n", msg.len, msg.data);
      e->evidence[i].data = NULL;
      e->evidence[i].len = 0;
    }

  e->cmlen = 0;
  e->elen = 0;
  e->cause_code = SUCCESS;
}

bool
error_equal (const error *left, const error *right)
{
  error_assert (left);
  error_assert (right);
  if (left->cause_code != right->cause_code)
    {
      return false;
    }

  if (left->cause_code)
    {
      if (left->cmlen != right->cmlen)
        {
          return false;
        }
      if (i_strncmp (left->cause_msg, right->cause_msg, left->cmlen) != 0)
        {
          return false;
        }
    }

  return true;
}
