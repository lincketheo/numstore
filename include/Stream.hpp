#pragma once

#include "types.hpp"

class Stream
{
public:
  virtual result<void> read_block (bbytes *dest) = 0;

  virtual result<void> read_once (bbytes *dest) = 0;

  virtual result<usize> discard_block (usize n) = 0;

  virtual result<usize> discard_once (usize n) = 0;

  virtual result<void> write (const bytes *src) = 0;
};
