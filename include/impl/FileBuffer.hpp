#pragma once

#include "Buffer.hpp"

class FileBuffer : public Buffer
{
public:
  FileBuffer (dtype type, int fd);

  ~FileBuffer ();

  result<usize> read (void *dest, usize dnelem, srange range) override;

  result<usize> append (const void *data, usize nelem) override;

private:
  int fd;
};
