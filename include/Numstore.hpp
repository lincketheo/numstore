#pragma once

#include "types.hpp"

#define SEG_LENGTH_B 2048

class DataSegment {
public:
  DataSegment();
  ~DataSegment();

private:
  usize length;
  usize data[SEG_LENGTH_B];
  DataSegment* next;

#define DataSegment_assert() \
  assert(length < SEG_LENGTH_B);
};

class TimeSegment {
public:
  TimeSegment();
  ~TimeSegment();

private:
  usize length;
  usize tstamp[SEG_LENGTH_B];
  TimeSegment* next;

#define TimeSegment_assert() \
  assert(length < SEG_LENGTH_B);
};

class Variable {
public:
  Variable(const char* name, usize nlen, dtype t, shape s);
  ~Variable();

  const char* name;
  usize nlen;
  dtype t;
  shape s;
  Variable* next;
  DataSegment* data_start;
  TimeSegment* time_start;

#define Variable_assert(v) \
  assert(v); \
  assert((v)->name); \
  assert((v)->nlen > 0); \
  shape_assert(&(v)->s);
};

class VariableFile {
public:
  VariableFile();
  ~VariableFile();

  result<Variable*> add_Variable(
    const char* name,
    usize nlen,
    dtype t,
    shape s);

  Variable* get_Variable(const char* name, usize nlen);

private:
  Variable* head;
};

