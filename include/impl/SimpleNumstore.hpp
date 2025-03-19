#pragma once

#include "Numstore.hpp"
#include "types.hpp"

typedef struct {

#define variable_assert(v) \
  assert(v); \
  assert((v)->vname); \
  assert((v)->vnamel > 0); \
  assert((v)->data); \
  assert((v)->dcap > 0); \
  shape_assert(&(v)->s);

  char* vname;
  usize vnamel;

  void* data;
  usize dcap;

  usize* time;
  usize tcap;

  usize len;

  shape s;
  dtype t;
} variable;

class SimpleNumstore : public Numstore {
public:
  SimpleNumstore();
  ~SimpleNumstore();

  result<void> define_variable(
    const char* vname,
    usize vnamel,
    shape s,
    dtype t
  ) override;

private:
  variable* vars;
  usize len;
  usize cap;
};
