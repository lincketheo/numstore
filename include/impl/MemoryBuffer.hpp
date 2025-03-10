#pragma once

#include "Buffer.hpp"

class MemoryBuffer : public Buffer
{
public:
  MemoryBuffer (dtype type, void *data, usize nelem, usize capelem);

  ~MemoryBuffer ();

  result<usize> read (void *dest, usize dnelem, srange range) const override;

  result<usize> append (const void *data, usize nelem) const override;

private:
  void *data;
  usize capelem;
  usize nelem;
};
