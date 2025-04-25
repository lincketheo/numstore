#include "dev/errors.h"
#include "intf/io.h"
#include "intf/logging.h"
#include "intf/mm.h"
#include "sds.h"

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

err_t
i_open (i_file *dest, const string fname, int read, int write)
{
  if (read && write)
    {
      dest->fd = open (fname.data, O_RDWR | O_CREAT, 0644);
    }
  else if (read)
    {
      dest->fd = open (fname.data, O_RDONLY | O_CREAT, 0644);
    }
  else if (write)
    {
      dest->fd = open (fname.data, O_WRONLY | O_CREAT, 0644);
    }
  else
    {
      ASSERT (0);
    }

  if (dest->fd == -1)
    {
      i_log_error ("Failed to open file: %s. Reason: %s\n",
                   fname.data, strerror (errno));
      return ERR_IO;
    }

  i_file_assert (dest);

  return SUCCESS;
}

err_t
i_close (i_file *fp)
{
  ASSERT (fp);
  i_file_assert (fp);
  int ret = close (fp->fd);
  if (ret)
    {
      i_log_error ("Failed to close file: %d. Reason: %s\n",
                   fp->fd, strerror (errno));
      return ERR_IO;
    }
  return SUCCESS;
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

err_t
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
          i_log_error ("Write failed to write %" PRIu64 " bytes. Reason: %s\n",
                       n, strerror (errno));
          return ERR_IO;
        }

      nwrite += _nwrite;
    }

  ASSERT (nwrite == n);

  return SUCCESS;
}

err_t
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

err_t
i_remove_quiet (const string fname)
{
  int ret = remove (fname.data);

  if (ret && errno != ENOENT)
    {
      i_log_error ("Failed to remove file: %s. Reason: %s\n",
                   fname.data, strerror (errno));
      return ERR_IO;
    }

  return SUCCESS;
}

err_t
i_access_rw (const string fname)
{
  if (access (fname.data, F_OK | W_OK | R_OK))
    {
      i_log_error ("failed to call access. Reason: %s\n",
                   strerror (errno));
      return ERR_IO;
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

err_t
i_mkstemp (i_file *dest, string tmpl)
{
  int fd = mkstemp (tmpl.data);
  if (fd == -1)
    {
      i_log_error ("Failed to call mkstemp for file: %s. Reason: %s\n",
                   tmpl.data, strerror (errno));
      return ERR_IO;
    }

  dest->fd = fd;
  return SUCCESS;
}

err_t
i_unlink (const string name)
{
  if (unlink (name.data))
    {
      i_log_error ("Failed to call unlink on file: %s. Reason: %s\n",
                   name.data, strerror (errno));
      return ERR_IO;
    }
  return SUCCESS;
}
