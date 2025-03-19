
#include "impl/SimpleNumstore.hpp"
#include "types.hpp"
#include <cassert>
#include <cstdlib>

static result<variable> variable_create(
  const char* vname,
  usize vnamel,
  shape s,
  dtype t) {

  variable ret;
  memset(&ret, 0, sizeof(ret));

  // Name
  ret.vname = (char*)malloc(vnamel);
  ret.vnamel = vnamel;
  if(ret.vname == nullptr) {
    goto failed;
  }

  // data
  ret.data = malloc(10 * s.size() * dtype_sizeof(t));
  ret.dcap = 10;
  if(ret.data == nullptr) {
    goto failed;
  }

  ret.time = (usize*)malloc(10 * sizeof*ret.time);
  ret.tcap = 10;
  if(ret.time == nullptr) {
    goto failed;
  }

  ret.len = 0;
  ret.s = s;
  ret.t = t;

  return ok(ret);

failed:
  if(ret.vname) { free(ret.vname); }
  if(ret.data) { free(ret.data); }
  if(ret.time) { free(ret.time); }
  return err<variable>();
}

static void variable_free(variable* v) {
  variable_assert(v);
  free(v->vname);
  free(v->data);
  free(v->time);
}


SimpleNumstore::SimpleNumstore() : vars(nullptr), len(0) {
}

SimpleNumstore::~SimpleNumstore() {
  if(vars) {
    assert(cap > 0);
    for(int i = 0; i < len; ++i) {
      variable_assert(&vars[i]);
      variable_free(&vars[i]);
    }
    free(vars);
  }
}

result<void> SimpleNumstore::define_variable(
  const char* vname,
  usize vnamel,
  shape s,
  dtype t) {

  result<variable> v = variable_create(vname, vnamel, s, t);
  if(!v.ok){
    return err<void>();
  }
  variable_assert(&v.value);

  // Initialize vars array
  if(len == 0) {
    assert(vars == nullptr);
    vars = (variable*)malloc(10 * sizeof*vars);
    if(vars == nullptr) {
      perror("malloc");
      variable_free(&v.value);
      return err<void>();
    }
    cap = 10;
    len = 0;
  }

  assert(vars != nullptr);
  assert(cap > 0);

  // Double array if needed
  if(len + 1 > cap) {
    variable* new_vars = (variable*)realloc(vars, 2 * cap * sizeof*new_vars);
    if(vars == nullptr) {
      perror("realloc");
      variable_free(&v.value);
      return err<void>();
    }
    cap = 2 * cap;
  }

  vars[len++] = v.value;

  return ok_void();
}
