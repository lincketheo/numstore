#pragma once

#include "Buffer.hpp"

class MemoryBuffer : public Buffer {
public:
  MemoryBuffer(dtype type, void* data, usize nelem, usize capelem);
  ~MemoryBuffer();

  result<usize> read (void *dest, usize dlen, srange range) override;

  result<usize> append (const void *data, usize nelem) override;

private:
  void* data;
  usize capelem;
  usize nelem;
};
