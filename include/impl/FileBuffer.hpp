#pragma once

#include "Buffer.hpp"

class FileBuffer : public Buffer
{
public:
  FileBuffer (dtype type, int fd);

  ~FileBuffer ();

  result<usize> read (void *dest, usize dnelem, srange range) const override;

  result<void> append (const void *data, usize nelem) const override;

private:
  int fd;
};
