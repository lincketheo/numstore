#include "string.hpp"
#include "types.hpp"

#include <cstdlib>

#define INITIAL_CAP 10

result<string> string_create() {
  char* data = (char*)malloc(INITIAL_CAP * sizeof *data);
  if(data == nullptr) {
    return err<string>();
  }

  return ok((string){
    .data = data,
    .len = 0,
    .cap = INITIAL_CAP,
  });
}

void string_free(string* s) {
  string_assert(s);
  free(s->data);
}

result<void> string::append(const char* _data, usize _len) {
  string_assert(this);
  if(_len == 0) {
    return ok_void();
  }

  assert(_data);
  assert(_len > 0);

  while(len + _len >= cap) {
    char* next = (char*)realloc(data, 2 * cap);
    if(next == nullptr) {
      return err<void>();
    }
    cap = 2 * cap;
    data = next;
  }

  memcpy(head(), _data, _len);
  len += _len;

  return ok_void();
}
