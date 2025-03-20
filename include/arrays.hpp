#pragma once

#include "types.hpp"
#include <stdlib.h>

template <typename T>
struct buffer {
  T* data;
  size_t cap;
  size_t len;
};

// Buffer behavior
// push, pop, 
template <typename T>
class Buffer {
public:
  virtual result<void> push_back(buffer<T> b) = 0;

  virtual result<void> push_front(buffer<T> b) = 0;

  virtual result<buffer<T>> pop_front(usize nelem) = 0;

  virtual result<buffer<T>> pop_back(usize nelem) = 0;
};

// Circular buffer
template <typename T>
class CBuffer : Buffer<T> {
  public:
  virtual result<buffer<T>> first_half() = 0;

  virtual result<buffer<T>> second_half() = 0;
};

// Dynamic buffer
template <typename T>
class DBuffer : Buffer<T> {
  public:
  buffer<T> data;
};

// Holds a circular buffer 
// and a dynamic buffer as backup
template <typename T>
class CDBuffer : Buffer<T> {
  public:
};
