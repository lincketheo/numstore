#include "errors/error.h"

#include "dev/assert.h"   // DEFINE_DBG_ASSERT_I
#include "intf/logging.h" // i_log_error
#include "intf/mm.h"      // lalloc
#include "intf/stdlib.h"  // i_vsnprintf
#include "utils/macros.h"

#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>

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
      ASSERT (e->evidence == NULL);
    }

  if (e->elen == 0)
    {
      ASSERT (e->evidence == NULL);
    }
  else
    {
      ASSERT (e->evidence);
    }

  /**
   * Don't allow evidence on null allocator
   */
  if (e->alloc == NULL)
    {
      ASSERT (e->elen == 0);
    }
}

error
error_create (lalloc *alloc)
{
  error ret = {
    .cause_code = SUCCESS,

    .cmlen = 0,

    .evidence = NULL,
    .elen = 0,
    .ecap = 0,
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
  u32 cmlen = i_vsnprintf (e->cause_msg, 256, fmt, ap);
  e->cmlen = MIN (cmlen, 255);
  va_end (ap);

  return err_t_from (e);
}

static inline err_t
error_make_room (error *e)
{
  error_assert (e);

  lalloc_r evidence;

  if (e->ecap == 0)
    {
      evidence = lmalloc (
          e->alloc,
          2, 1, sizeof *e->evidence);
    }
  else
    {
      evidence = lrealloc (
          e->alloc,
          e->evidence,
          e->ecap * 1.25, (e->ecap + 1), sizeof *e->evidence);
    }

  switch (evidence.stat)
    {
    case AR_SUCCESS:
      {
        e->ecap = evidence.rlen;
        e->evidence = evidence.ret;
        break;
      }
    case AR_NOMEM:
    case AR_AVAIL_BUT_USED:
      {
        return ERR_NOMEM;
      }
    }

  return SUCCESS;
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

  if (e->ecap == e->elen)
    {
      err_t ret = error_make_room (e);
      /**
       * Sweep malloc errors under the rug
       */
      if (ret)
        {
          return err_t_from (e);
        }
    }

  va_list ap;
  va_start (ap, fmt);
  u32 n = i_vsnprintf (NULL, 0, fmt, ap) + 1;
  va_end (ap);

  lalloc_r buf = lmalloc (e->alloc, n, n, 1);

  switch (buf.stat)
    {
    case AR_SUCCESS:
      {
        va_start (ap, fmt);
        i_vsnprintf (buf.ret, buf.rlen, fmt, ap);
        e->evidence[e->elen++] = (string){
          .data = buf.ret,
          .len = buf.rlen,
        };
        return err_t_from (e);
      }
    case AR_NOMEM:
    case AR_AVAIL_BUT_USED:
      {
        return err_t_from (e);
      }
    default:
      {
        UNREACHABLE ();
        return ERR_FALLBACK;
      }
    }
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
  e->cmlen = 0;

  for (u32 i = 0; i < e->elen; ++i)
    {
      string msg = e->evidence[i];
      i_log_warn ("TRACEBACK: %.*s\n", msg.len, msg.data);
      lfree (e->alloc, msg.data);
      e->evidence[i].data = NULL;
      e->evidence[i].len = 0;
    }

  e->elen = 0;
  if (e->ecap > 0)
    {
      lfree (e->alloc, e->evidence);
      e->ecap = 0;
      e->evidence = NULL;
    }

  e->cause_code = SUCCESS;
}

#ifndef NTEST
void
error_reset (error *e)
{
  error_assert (e);
  ASSERT (e->cause_code != SUCCESS);

  e->cmlen = 0;

  for (u32 i = 0; i < e->elen; ++i)
    {
      lfree (e->alloc, e->evidence[i].data);
      e->evidence[i].data = NULL;
      e->evidence[i].len = 0;
    }
  e->elen = 0;
  if (e->ecap > 0)
    {
      lfree (e->alloc, e->evidence);
      e->ecap = 0;
      e->evidence = NULL;
    }
}
#endif
