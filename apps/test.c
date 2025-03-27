#include "testing.h"

int main() {
  for (u64 i = 0; i < ntests; ++i) {
    tests[i]();
  }
  return test_ret;
}
