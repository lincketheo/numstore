#include "Numstore.hpp"
#include "types.hpp"
#include "utils.hpp"
#include "testing.hpp"
#include <iterator>
#include <new>

DataSegment::DataSegment() {
  length = 0;
  next = nullptr;
}

DataSegment::~DataSegment() {
  DataSegment_assert();
  if(next) {
    delete next;
  }
}

TimeSegment::TimeSegment() {
  length = 0;
  next = nullptr;
}

TimeSegment::~TimeSegment() {
  TimeSegment_assert();
  if(next) {
    delete next;
  }
}

Variable::Variable(
  const char* _name,
  usize _nlen,
  dtype _t,
  shape _s
) {
  name = _name;
  nlen = _nlen;
  t = _t;
  s = _s;
  time_start = nullptr;
  data_start = nullptr;

  Variable_assert(this);
}

Variable::~Variable() {
  Variable_assert(this);

  if(next) {
    delete next;
  }

  if(data_start) {
    delete data_start;
  }

  if(time_start) {
    delete time_start;
  }

  delete[] name;
}

static result<Variable*> Variable_create(
  const char* name,
  usize nlen,
  dtype t,
  shape s
) {
  assert(name);
  assert(nlen > 0);
  shape_assert(&s);

  Variable* ret;
  char* rname;

  // Create new name
  rname = (char*)new (std::nothrow) char[nlen];
  if(rname == nullptr) {
    perror("new");
    goto failed;
  }
  std::memcpy(rname, name, nlen);

  // Create return value
  ret = new (std::nothrow) Variable(rname, nlen, t, s);
  if(ret == nullptr) {
    perror("new");
    goto failed;
  }

  Variable_assert(ret);

  return ok<Variable*>(ret);

failed:
  if(rname) {
    delete [] rname;
  }
  if(ret) {
    delete [] ret;
  }
  return err<Variable*>();
}

static inline int Variable_cmp(const Variable *a, const Variable *b) {
  Variable_assert(a);
  Variable_assert(b);

  size_t n = MIN(a->nlen, b->nlen);
  int cmp = strncmp(a->name, b->name, n);

  if (cmp == 0) {
    if (a->nlen == b->nlen){
      return 0;
    }
    return (a->nlen < b->nlen) ? -1 : 1;
  }
  return cmp;
}

static result<Variable*> push_sorted(Variable* head, Variable* v) {
  Variable_assert(v);

  v->next = NULL;

  if (head == NULL){
    return ok(v);
  }

  Variable_assert(head);

  int cmp = Variable_cmp(v, head);
  if (cmp == 0){
    return err<Variable*>();  // Collision
  }

  if (cmp < 0) {
    v->next = head;
    return ok(v);
  }

  Variable *current = head;
  while (current->next != NULL) {
    cmp = Variable_cmp(v, current->next);

    if (cmp == 0){
      return err<Variable*>();  // Collision
    }

    if (cmp < 0){
      break;
    }

    current = current->next;
  }

  v->next = current->next;
  current->next = v;
  return ok(head);
}

VariableFile::VariableFile() {
  head = nullptr;
}

VariableFile::~VariableFile() {
  if(head) {
    delete head;
  }
}

result<Variable*> VariableFile::add_Variable(
  const char* name,
  usize nlen,
  dtype t,
  shape s) {

  result<Variable*> nVariable;
  result<Variable*> nhead;

  nVariable = Variable_create(name, nlen, t, s);
  if(!nVariable.ok) {
    goto failed;
  }

  nhead = push_sorted(head, nVariable.value);
  if(!nhead.ok) {
    goto failed;
  }
  head = nhead.value;

  return ok(nVariable.value);

failed:
  if(nVariable.ok) {
    delete nVariable.value;
  }
  return err<Variable*>();
}

Variable* VariableFile::get_Variable(const char* name, usize nlen) {
  assert(name);
  assert(nlen > 0);

  Variable* temp = head;

  while(temp->nlen != nlen || strncmp(temp->name, name, nlen) != 0) {
    temp = temp->next;
  }

  return temp;
}


TEST(VariableFile) {
  VariableFile f;

  usize dims[2] = {1, 2};
  shape s = {.dims = dims, .rank = 2};

  result<Variable*> v = f.add_Variable("foo", 3, U32, s);
  test_assert_equal(v.ok, 1, "%d");
  test_assert_equal(v.value->nlen, 3lu, "%zu");

  v = f.add_Variable("barr", 4, U32, s);
  test_assert_equal(v.ok, 1, "%d");
  test_assert_equal(v.value->nlen, 4lu, "%zu");

  v = f.add_Variable("bizzz", 5, U32, s);
  test_assert_equal(v.ok, 1, "%d");
  test_assert_equal(v.value->nlen, 5lu, "%zu");
}
