#include "errors/error.h"

#include "dev/assert.h"   // DEFINE_DBG_ASSERT_I
#include "intf/logging.h" // i_log_error
#include "intf/mm.h"      // lalloc
#include "intf/stdlib.h"  // i_vsnprintf

#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>

DEFINE_DBG_ASSERT_I (error, error, e)
{
  ASSERT (e);
  if (e->is_error)
    {
      ASSERT (e->cause_code != SUCCESS);
    }
  if (e->cause_msg.len == 0)
    {
      ASSERT (e->cause_msg.data == NULL);
    }
  else
    {
      ASSERT (e->cause_msg.data);
    }
  if (e->elen == 0)
    {
      ASSERT (e->evidence == NULL);
    }
  else
    {
      ASSERT (e->evidence);
    }
}

error
error_create (lalloc *alloc)
{
  error ret = {
    .is_error = false,
    .cause_code = SUCCESS,
    .cause_msg.len = 0,
    .cause_msg.data = NULL,
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
  if (e == NULL)
    {
      return c;
    }

  error_assert (e);
  ASSERT (!e->is_error);

  e->is_error = true;
  e->cause_code = c;

  if (fmt == NULL)
    {
      return c;
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
        vsnprintf (buf.ret, buf.rlen, fmt, ap);
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

static inline err_t
error_make_room (error *e)
{
  error_assert (e);

  lalloc_r evidence = lrealloc (
      e->alloc,
      e->evidence,
      e->ecap * 2,
      (e->ecap + 1),
      sizeof *e->evidence);

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
  ASSERT (!e->is_error);

  if (fmt == NULL)
    {
      return err_t_from (e);
    }

  if (e->ecap == e->elen)
    {
      err_t ret = error_make_room (e);
      if (ret)
        {
          return e->cause_code;
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
        vsnprintf (buf.ret, buf.rlen, fmt, ap);
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

  i_log_error ("%.*s\n", e->cause_msg.len, e->cause_msg.data);
  lfree (e->alloc, e->cause_msg.data);
  e->cause_msg.data = NULL;
  e->cause_msg.len = 0;

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
}

#ifndef NTEST
void
error_reset (error *e)
{
  error_assert (e);
  ASSERT (e->cause_code != SUCCESS);

  if (e->cause_code != SUCCESS)
    {
      lfree (e->alloc, e->cause_msg.data);
      e->cause_msg.data = NULL;
      e->cause_msg.len = 0;
    }

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
