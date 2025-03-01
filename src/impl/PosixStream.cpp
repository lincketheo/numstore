#include "impl/PosixStream.hpp"

#include <unistd.h>

PosixStream::PosixStream (int _fd) { fd = _fd; }

result<void>
PosixStream::read_once (bbytes *dest)
{
  ssize_t ret = read (fd, dest->head (), dest->avail ());

  if (ret == -1)
    {
      return err<void> ();
    }

  dest->update_add ((size_t)ret);

  return ok_void ();
}
