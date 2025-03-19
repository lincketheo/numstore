#pragma once

#include "argument.hpp"
#include "types.hpp"

class Numstore {
  public:
  virtual result<void> write(write_argument arg);
};
