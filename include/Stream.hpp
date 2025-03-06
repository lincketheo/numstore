#pragma once

#include "types.hpp"

class Stream
{
public:
  Stream(dtype type);
  ~Stream();

  // Discards elements that don't lie in range
  virtual result<void> read (void *dest, usize dlen, srange range) = 0;

  // Writes data to the stream
  virtual result<void> append (const void* src, usize nelem) = 0;

  // Stream is done
  virtual result<bool> done() = 0;

protected:
  const dtype type;
};
