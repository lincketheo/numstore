#pragma once

#include "Dataset.hpp"

class FDDataset : public Dataset
{
public:
  FDDataset (dtype type, int fd);

  result<usize> read (void *dest, usize dlen, srange range) override;
  result<usize> append (const void *data, usize nelem) override;

private:
  int fd;
};
