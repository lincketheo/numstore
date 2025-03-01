#pragma once

#include "File.hpp"

class PosixFile : File
{
public:
  PosixFile(int fd);

  virtual result<usize> size () override;

  virtual result<void> read (bbytes *dest, usize start) override;

  virtual result<void> write (const bytes *src, usize start) override;

  virtual result<void> append (const bytes *src) override;

  virtual result<void> mmap (bbytes *dest, usize start) override;
};
