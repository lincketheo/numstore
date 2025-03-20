#include <stdlib.h>

template <typename T>
struct buffer {
  T* data;
  size_t cap;
  size_t len;
};

template <typename T>
class CBuffer {
  public:
  CBuffer(T* data, size_t cap);

  

  private:
  T* data;
  size_t cap;
  size_t len;
  size_t head;
  size_t tail;
};

template <typename T>
class DBuffer {
  public:

  T* data;
private:
  size_t cap;
  size_t len;
};

template <typename T>
class ExpandingBuffer<T> {
  public:
  ExpandingBuffer();

  private:
  CBuffer<T> window;
};
