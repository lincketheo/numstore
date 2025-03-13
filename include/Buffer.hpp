#pragma once

#include "types.hpp"

class Buffer
{
public:
  Buffer (dtype _type);
  ~Buffer () = default;

  // Read [range] from source into [dest]
  // Does not change internal data
  virtual result<usize> read (void *dest, usize dnelem, srange range) const = 0;

  // Write [nelem] elements from [data] into source
  // Does not change internal data
  virtual result<void> append (const void *data, usize nelem) const = 0;

protected:
  const dtype type;
};
