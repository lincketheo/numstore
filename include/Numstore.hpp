#pragma once

#include "types.hpp"

class Numstore {
  public:
  virtual result<void> define_variable(
    const char* vname, 
    usize vnamel,
    shape s,
    dtype t
  ) = 0;
};
