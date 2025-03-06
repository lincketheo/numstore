#pragma once

#include "types.hpp"

class Buffer
{
public:
  Buffer (dtype type);
  ~Buffer();

  // Read [range] from source into [dest]
  virtual result<usize> read (void *dest, usize dlen, srange range) = 0;

  // Write [nelem] elements from [data] into source
  virtual result<usize> append (const void *data, usize nelem) = 0;

protected:
  const dtype type;
};

