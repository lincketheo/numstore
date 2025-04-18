#include "dev/errors.h"
#include "os/io.h"
#include "os/logging.h"
#include "os/mm.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#ifndef NDEBUG
static u64 num_read = 0;
static u64 num_write = 0;
static u64 num_alloc = 0;
static u64 num_open = 0;
static u64 num_truncate = 0;

void
io_log_stats ()
{
  i_log_info ("Total open calls: %zu\n", num_open);
  i_log_info ("Total read calls: %zu\n", num_read);
  i_log_info ("Total write calls: %zu\n", num_write);
  i_log_info ("Total alloc calls: %zu\n", num_alloc);
  i_log_info ("Total truncate calls: %zu\n", num_alloc);
}
#endif

/////////////////////// Files
struct i_file
{
  int fd;
};

DEFINE_DBG_ASSERT_I (i_file, i_file, fp)
{
  ASSERT (fp);
  ASSERT (fcntl (fp->fd, F_GETFD) != -1 || errno != EBADF);
}

i_file *
i_open (const string fname, int read, int write)
{
  cstring_assert (&fname);

  i_file *ret = i_malloc (sizeof ret);

  if (ret == NULL)
    {
      return ret;
    }

#ifndef NDEBUG
  num_open++;
#endif

  if (read && write)
    {
      ret->fd = open (fname.data, O_RDWR | O_CREAT, 0644);
    }
  else if (read)
    {
      ret->fd = open (fname.data, O_RDONLY | O_CREAT, 0644);
    }
  else if (write)
    {
      ret->fd = open (fname.data, O_WRONLY | O_CREAT, 0644);
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
                   fname.data, strerror (errno));
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

#ifndef NDEBUG
  num_read++;
#endif

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

#ifndef NDEBUG
      num_read++;
#endif

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

#ifndef NDEBUG
  num_write++;
#endif

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

#ifndef NDEBUG
      num_write++;
#endif

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
#ifndef NDEBUG
  num_truncate++;
#endif

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

int
i_remove_quiet (const string fname)
{
  cstring_assert (&fname);
  int ret = remove (fname.data);

  if (ret && errno != ENOENT)
    {
      i_log_error ("Failed to remove file: %s\n", fname.data);
      return ERR_IO;
    }

  return SUCCESS;
}
