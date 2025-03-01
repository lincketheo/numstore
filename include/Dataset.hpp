#pragma once

#include "types.hpp"

class Dataset
{
public:
  Dataset (dtype type);
  ~Dataset ();

  virtual result<usize> read (void *dest, usize dlen, srange range) = 0;
  virtual result<usize> append (const void *data, usize nelem) = 0;

protected:
  const dtype type;
};

