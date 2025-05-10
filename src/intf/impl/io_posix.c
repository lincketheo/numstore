#include "ds/strings.h"
#include "errors/error.h"
#include "intf/io.h"
#include "intf/logging.h"
#include "intf/mm.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

/////////////////////// Files

DEFINE_DBG_ASSERT_I (i_file, i_file, fp)
{
  ASSERT (fp);
  ASSERT (fcntl (fp->fd, F_GETFD) != -1 || errno != EBADF);
}

////////////////// Open / Close
err_t
i_open_rw (i_file *dest, const string fname, error *e)
{
  int fd = open (fname.data, O_RDWR | O_CREAT, 0644);

  if (fd == -1)
    {
      error_causef (e, ERR_IO, "open_rw %.*s: %s",
                    fname.len, fname.data, strerror (errno));
      return err_t_from (e);
    }

  i_file_assert (dest);

  return SUCCESS;
}

err_t
i_close (i_file *fp, error *e)
{
  ASSERT (fp);
  i_file_assert (fp);
  int ret = close (fp->fd);
  if (ret)
    {
      error_causef (e, ERR_IO, "close: %s", strerror (errno));
      error_trailf_dbg (e, "fd: %d", fp->fd);
      return err_t_from (e);
    }
  return SUCCESS;
}

////////////////// Positional Read / Write
i64
i_pread_some (i_file *fp, void *dest, u64 n, u64 offset, error *e)
{
  i_file_assert (fp);
  ASSERT (dest);
  ASSERT (n > 0);

  ssize_t ret = pread (fp->fd, dest, n, (size_t)offset);

  if (ret < 0 && errno != EINTR)
    {
      error_causef (e, ERR_IO, "pread: %s", strerror (errno));
      error_trailf_dbg (e, "fd: %d n: %" PRIu64 " offset: %" PRIu64 ".", fp->fd, n, offset);
      return err_t_from (e);
    }

  return (i64)ret;
}

i64
i_pread_all (i_file *fp, void *dest, u64 n, u64 offset, error *e)
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
      if (_nread < 0 && errno != EINTR)
        {
          error_causef (e, ERR_IO, "pread: %s", strerror (errno));
          error_trailf_dbg (
              e, "fd: %d n: %" PRIu64 " offset: %" PRIu64 " return: %lu.",
              fp->fd, n - nread, offset, _nread);
          return err_t_from (e);
        }

      nread += (i64)_nread;
    }

  ASSERT (nread == n);

  return nread;
}

i64
i_pwrite_some (i_file *fp, const void *src, u64 n, u64 offset, error *e)
{
  i_file_assert (fp);
  ASSERT (src);
  ASSERT (n > 0);

  ssize_t ret = pwrite (fp->fd, src, n, (size_t)offset);
  if (ret < 0 && errno != EINTR)
    {
      error_causef (e, ERR_IO, "pread: %s", strerror (errno));
      error_trailf_dbg (e, "n: %" PRIu64 " offset: %" PRIu64, n, offset);
      return err_t_from (e);
    }
  return (i64)ret;
}

err_t
i_pwrite_all (i_file *fp, const void *src, u64 n, u64 offset, error *e)
{
  i_file_assert (fp);
  ASSERT (src);
  ASSERT (n > 0);

  u8 *_src = (u8 *)src;
  u64 nwrite = 0;

  while (nwrite < n)
    {
      // Do write
      ASSERT (n > nwrite);
      ssize_t _nwrite = pwrite (
          fp->fd,
          _src + nwrite,
          n - nwrite,
          offset + nwrite);

      // Error
      if (_nwrite < 0 && errno != EINTR)
        {
          error_causef (e, ERR_IO, "pwrite: %s", strerror (errno));
          error_trailf_dbg (
              e, "n: %" PRIu64 " offset: %" PRIu64, n, offset);
          return err_t_from (e);
        }

      nwrite += _nwrite;
    }

  ASSERT (nwrite == n);

  return SUCCESS;
}

////////////////// Stream Read / Write
i64
i_read_some (i_file *fp, void *dest, u64 nbytes, error *e)
{
  i_file_assert (fp);
  ASSERT (dest);
  ASSERT (nbytes > 0);

  ssize_t ret = read (fp->fd, dest, nbytes);
  if (ret < 0 && errno != EINTR)
    {
      error_causef (e, ERR_IO, "read: %s", strerror (errno));
      error_trailf_dbg (e, "nbytes: %" PRIu64, nbytes);
      return err_t_from (e);
    }
  return (i64)ret;
}

i64
i_read_all (i_file *fp, void *dest, u64 nbytes, error *e)
{
  i_file_assert (fp);
  ASSERT (dest);
  ASSERT (nbytes > 0);

  u8 *_dest = (u8 *)dest;
  u64 nread = 0;

  while (nread < nbytes)
    {
      // Do read
      ASSERT (nbytes > nread);
      ssize_t _nread = read (
          fp->fd,
          _dest + nread,
          nbytes - nread);

      // EOF
      if (_nread == 0)
        {
          return (i64)nread;
        }

      // Error
      if (_nread < 0 && errno != EINTR)
        {
          error_causef (e, ERR_IO, "read: %s", strerror (errno));
          error_trailf_dbg (e, "nbytes: %" PRIu64 " return: %lu", nbytes, _nread);
          return err_t_from (e);
        }

      nread += (i64)_nread;
    }

  ASSERT (nread == nbytes);

  return nread;
}

i64
i_write_some (i_file *fp, const void *src, u64 nbytes, error *e)
{
  i_file_assert (fp);
  ASSERT (src);
  ASSERT (nbytes > 0);

  ssize_t ret = write (fp->fd, src, nbytes);
  if (ret < 0 && errno != EINTR)
    {
      error_causef (e, ERR_IO, "write: %s", strerror (errno));
      error_trailf_dbg (e, "nbytes: %" PRIu64 " return: %lu", nbytes, ret);
      return err_t_from (e);
    }
  return (i64)ret;
}

err_t
i_write_all (i_file *fp, const void *src, u64 nbytes, error *e)
{
  i_file_assert (fp);
  ASSERT (src);
  ASSERT (nbytes > 0);

  u8 *_src = (u8 *)src;
  u64 nwrite = 0;

  while (nwrite < nbytes)
    {
      // Do read
      ASSERT (nbytes > nwrite);

      ssize_t _nwrite = write (
          fp->fd,
          _src + nwrite,
          nbytes - nwrite);

      // Error
      if (_nwrite < 0 && errno != EINTR)
        {
          error_causef (e, ERR_IO, "write: %s", strerror (errno));
          error_trailf_dbg (e, "nbytes: %" PRIu64 " return: %lu", nbytes, _nwrite);
          return err_t_from (e);
        }

      nwrite += _nwrite;
    }

  ASSERT (nwrite == nbytes);

  return SUCCESS;
}

////////////////// Others
err_t
i_truncate (i_file *fp, u64 bytes, error *e)
{
  if (ftruncate (fp->fd, bytes) == -1)
    {
      error_causef (e, ERR_IO, "truncate: %s", strerror (errno));
      error_trailf_dbg (e, "bytes: %" PRIu64, bytes);
      return err_t_from (e);
    }

  return 0;
}

i64
i_file_size (i_file *fp, error *e)
{
  struct stat st;
  if (fstat (fp->fd, &st) == -1)
    {
      error_causef (e, ERR_IO, "fstat: %s", strerror (errno));
      return err_t_from (e);
    }
  return (off_t)st.st_size;
}

err_t
i_remove_quiet (const string fname, error *e)
{
  int ret = remove (fname.data);

  if (ret && errno != ENOENT)
    {
      error_causef (e, ERR_IO, "truncate: %s", strerror (errno));
      return err_t_from (e);
    }

  return SUCCESS;
}

err_t
i_mkstemp (i_file *dest, string tmpl, error *e)
{
  int fd = mkstemp (tmpl.data);
  if (fd == -1)
    {
      error_causef (e, ERR_IO, "mkstemp: %s", strerror (errno));
      return err_t_from (e);
    }

  dest->fd = fd;
  return SUCCESS;
}

err_t
i_unlink (const string name, error *e)
{
  if (unlink (name.data))
    {
      error_causef (e, ERR_IO, "unlink: %s", strerror (errno));
      return err_t_from (e);
    }
  return SUCCESS;
}

////////////////// Wrappers
err_t
i_access_rw (const string fname, error *e)
{
  if (access (fname.data, F_OK | W_OK | R_OK))
    {
      error_causef (e, ERR_IO, "access: %s", strerror (errno));
      return err_t_from (e);
    }
  return SUCCESS;
}

bool
i_exists_rw (const string fname)
{
  if (access (fname.data, F_OK | W_OK | R_OK))
    {
      return false;
    }
  return true;
}
