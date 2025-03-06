#include "impl/MemoryBuffer.hpp"

#include <cassert>

MemoryBuffer::MemoryBuffer(dtype type, void* _data, usize _nelem, usize _capelem) : Buffer(type) {
  assert(_nelem <= _capelem);
  data = _data;
  capelem = _capelem;
  nelem = _nelem;
}

MemoryBuffer::~MemoryBuffer() {}

result<usize> MemoryBuffer::read (void *dest, usize dlen, srange range) {
    
}
