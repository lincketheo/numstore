#pragma once

#include "types.hpp"

class File
{
public:
  virtual result<usize> size () = 0;

  virtual result<void> read (bbytes *dest, usize start) = 0;

  virtual result<void> write (const bytes *src, usize start) = 0;

  virtual result<void> append (const bytes *src) = 0;

  virtual result<void> mmap (bbytes *dest, usize start) = 0;
};

