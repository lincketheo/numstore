#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "common/macros.h"
#include "common/types.h"
#include "dev/assert.h"
#include "dev/errors.h"
#include "os/io.h"

/////////////////////// Allocation
void *
i_malloc (u64 bytes)
{
  ASSERT (bytes > 0);
  void *ret = malloc ((size_t)bytes);
  if (ret == NULL)
    {
      i_log_error ("Failed to allocate %zu bytes. Reason: %s", bytes,
                   strerror (errno));
    }
  return ret;
}

void *
i_calloc (u64 n, u64 size)
{
  ASSERT (size > 0);
  ASSERT (n > 0);
  void *ret = calloc ((size_t)n, (size_t)size);
  if (ret == NULL)
    {
      i_log_error ("Failed to c-allocate %zu bytes. Reason: %s", size * n,
                   strerror (errno));
    }
  return ret;
}

void
i_free (void *ptr)
{
  ASSERT (ptr);
  free (ptr);
}

void *
i_realloc (void *ptr, int bytes)
{
  ASSERT (ptr);
  ASSERT (bytes > 0);
  void *ret = realloc (ptr, bytes);
  if (ret == NULL)
    {
      i_log_error ("Failed to reallocate %zu bytes. Reason: %s", bytes,
                   strerror (errno));
    }
  return ret;
}

/////////////////////// Files
struct i_file
{
  int fd;
};

DEFINE_DBG_ASSERT (i_file, i_file, fp)
{
  ASSERT (fp);
  ASSERT (fcntl (fp->fd, F_GETFD) != -1 || errno != EBADF);
}

i_file *
i_open (const string *fname, int read, int write)
{
  ASSERT (fname);

  i_file *ret = i_malloc (sizeof ret);

  if (ret == NULL)
    {
      return ret;
    }

  cstring_assert (fname);

  if (read && write)
    {
      ret->fd = open (fname->data, O_RDWR | O_CREAT, 0644);
    }
  else if (read)
    {
      ret->fd = open (fname->data, O_RDONLY | O_CREAT, 0644);
    }
  else if (write)
    {
      ret->fd = open (fname->data, O_WRONLY | O_CREAT, 0644);
    }
  else
    {
      i_log_error ("Refusing to open file: %s "
                   "without either read or write\n",
                   fname);
      return NULL;
    }

  if (ret->fd == -1)
    {
      i_log_error ("Failed to open file: %s. Reason: %s\n",
                   fname->data, strerror (errno));
      i_free (ret);
      return NULL;
    }

  i_file_assert (ret);

  return ret;
}

int
i_close (i_file *fp)
{
  ASSERT (fp);
  i_file_assert (fp);
  return close (fp->fd);
}

i64
i_read_some (i_file *fp, void *dest, u64 n, u64 offset)
{
  i_file_assert (fp);
  ASSERT (dest);
  ASSERT (n > 0);
  ssize_t ret = pread (fp->fd, dest, n, (size_t)offset);
  return (i64)ret;
}

i64
i_read_all (i_file *fp, void *dest, u64 n, u64 offset)
{
  i_file_assert (fp);
  ASSERT (dest);
  ASSERT (n > 0);

  u8 *_dest = (u8 *)dest;
  u64 nread = 0;

  while (nread < n)
    {
      // Do read
      ASSERT (n > nread);
      ssize_t _nread = pread (
          fp->fd,
          _dest + nread,
          n - nread,
          offset + nread);

      // EOF
      if (_nread == 0)
        {
          return (i64)nread;
        }

      // Error
      if (_nread < 0)
        {
          if (errno == EINTR)
            {
              continue;
            }
          return _nread;
        }

      nread += (i64)_nread;
    }

  ASSERT (nread == n);

  return nread;
}

i64
i_write_some (i_file *fp, const void *src, u64 n, u64 offset)
{
  i_file_assert (fp);
  ASSERT (src);
  ASSERT (n > 0);
  ssize_t ret = pwrite (fp->fd, src, n, (size_t)offset);
  return (i64)ret;
}

int
i_write_all (i_file *fp, const void *src, u64 n, u64 offset)
{
  i_file_assert (fp);
  ASSERT (src);
  ASSERT (n > 0);

  u8 *_src = (u8 *)src;
  u64 nwrite = 0;

  while (nwrite < n)
    {
      // Do read
      ASSERT (n > nwrite);
      ssize_t _nwrite = pwrite (
          fp->fd,
          _src + nwrite,
          n - nwrite,
          offset + nwrite);

      // IEEE Std 1003.1 2017
      // Assuming write = 0 with
      // requested > 0 is an error
      if (_nwrite == 0)
        {
          return ERR_IO;
        }

      // Error
      if (_nwrite < 0)
        {
          if (errno == EINTR)
            {
              continue;
            }
          i_log_error ("Write failed to write %zu bytes. Reason: %s\n",
                       strerror (errno));
          return ERR_IO;
        }

      nwrite += _nwrite;
    }

  ASSERT (nwrite == n);

  return SUCCESS;
}

int
i_truncate (i_file *fp, u64 bytes)
{
  if (ftruncate (fp->fd, bytes) == -1)
    {
      i_log_error ("Failed to call ftruncate. Reason: %s\n",
                   strerror (errno));
      return ERR_IO;
    }

  return 0;
}

i64
i_file_size (i_file *fp)
{
  struct stat st;
  if (fstat (fp->fd, &st) == -1)
    {
      i_log_error ("Failed to call fstat. Reason: %s\n",
                   strerror (errno));
      return ERR_IO;
    }
  return (off_t)st.st_size;
}

/////////////////////// Logging
#ifndef NLOGGING
static void
i_log_internal (const char *prefix, const char *color, const char *fmt, va_list args)
{
  fprintf (stderr, "%s%s: ", color, prefix);
  vfprintf (stderr, fmt, args);
  fprintf (stderr, "%s\n", RESET);
}

//// Log level wrappers
void
i_log_trace (const char *fmt, ...)
{
  va_list args;
  va_start (args, fmt);
  i_log_internal ("TRACE", BOLD_BLACK, fmt, args);
  va_end (args);
}

void
i_log_debug (const char *fmt, ...)
{
  va_list args;
  va_start (args, fmt);
  i_log_internal ("DEBUG", BLUE, fmt, args);
  va_end (args);
}

void
i_log_info (const char *fmt, ...)
{
  va_list args;
  va_start (args, fmt);
  i_log_internal ("INFO", GREEN, fmt, args);
  va_end (args);
}

void
i_log_warn (const char *fmt, ...)
{
  va_list args;
  va_start (args, fmt);
  i_log_internal ("WARN", YELLOW, fmt, args);
  va_end (args);
}

void
i_log_error (const char *fmt, ...)
{
  va_list args;
  va_start (args, fmt);
  i_log_internal ("ERROR", RED, fmt, args);
  va_end (args);
}

void
i_log_failure (const char *fmt, ...)
{
  va_list args;
  va_start (args, fmt);
  i_log_internal ("FAILURE", BOLD_RED, fmt, args);
  va_end (args);
}

void
i_log_success (const char *fmt, ...)
{
  va_list args;
  va_start (args, fmt);
  i_log_internal ("SUCCESS", BOLD_GREEN, fmt, args);
  va_end (args);
}
#endif
