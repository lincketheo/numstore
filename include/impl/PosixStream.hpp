#pragma once

#include "Stream.hpp"

class PosixStream : Stream
{
public:
  PosixStream (int fd);

  virtual result<void> read_once (bbytes *dest) override;

  virtual result<usize> discard_once (usize n) override;

  virtual result<void> write (const bytes *src) override;

private:
  int fd;
};
