#pragma once

#include <cassert>
#include <complex>
#include <cstdint>
#include <cstring>

/////// DTYPES
typedef std::uint8_t u8;
typedef std::uint16_t u16;
typedef std::uint32_t u32;
typedef std::uint64_t u64;

typedef std::int8_t i8;
typedef std::int16_t i16;
typedef std::int32_t i32;
typedef std::int64_t i64;

typedef float f32;
typedef double f64;
typedef long double f128;

typedef std::complex<f32> cf64;
typedef std::complex<f64> cf128;
typedef std::complex<f128> cf256;

typedef std::complex<i8> ci16;
typedef std::complex<i16> ci32;
typedef std::complex<i32> ci64;
typedef std::complex<i64> ci128;

typedef std::complex<u8> cu16;
typedef std::complex<u16> cu32;
typedef std::complex<u32> cu64;
typedef std::complex<u64> cu128;

typedef std::size_t usize;
typedef long ssize;

#ifdef SIZE_T_MAX
#define USIZE_MAX SIZE_T_MAX
#else
#define USIZE_MAX SIZE_MAX
#endif

typedef enum
{
  U8,
  U16,
  U32,
  U64,
  I8,
  I16,
  I32,
  I64,
  F32,
  F64,
  F128,
  CF64,
  CF128,
  CF256,
  CI16,
  CI32,
  CI64,
  CI128,
  CU16,
  CU32,
  CU64,
  CU128
} dtype;

static inline size_t
dtype_sizeof (const dtype type)
{
  switch (type)
    {
    case U8:
    case I8:
      return 1;
    case U16:
    case I16:
      return 2;
    case U32:
    case I32:
    case F32:
      return 4;
    case U64:
    case I64:
    case F64:
      return 8;
    case F128:
      return 16;
    case CF64:
    case CI16:
    case CU16:
      return 8;
    case CF128:
    case CI32:
    case CU32:
      return 16;
    case CF256:
    case CI64:
    case CU64:
      return 32;
    case CI128:
    case CU128:
      return 64;
    }
  return 0;
}

/////// RANGES
typedef struct
{
  usize start;
  usize end;
  usize span;
} srange;

#define srange_assert(s) assert ((s)->start <= (s)->end);

// Copies data from [src] into [dest] using range defined by [range]
// [dnelem] is the capacity of dest - data is appended contiguously
// [snelem] is the number of elements in src
usize srange_copy (u8 *dest, usize dnelem, const u8 *src, usize snelem,
                   srange range, usize size);

/////// Bytes
typedef struct
{
  void *data;
  usize len;
} bytes;

#define bbytes_assert(b)                                                      \
  assert ((b)->data);                                                         \
  assert ((b)->cap >= (b)->len)

// buffered bytes
struct bbytes
{
  void *data;
  usize len;
  usize cap;

  // Gets the number of bytes
  // available
  inline usize
  avail ()
  {
    bbytes_assert (this);
    return cap - len;
  }

  // Returns the head byte
  inline void *
  head ()
  {
    bbytes_assert (this);
    return (void *)((u8 *)data + len);
  }

  // Increments len
  inline void
  update_add (usize amnt)
  {
    bbytes_assert (this);
    len += amnt;
    bbytes_assert (this);
  }

  // Decrements len
  inline void
  update_remove (usize amnt)
  {
    bbytes_assert (this);
    assert (len >= amnt);
    len -= amnt;
    bbytes_assert (this);
  }
};

/////// RESULTS
template <typename T> struct result
{
  T value;
  bool ok;
};

template <> struct result<void>
{
  bool ok;
};

template <typename T>
static inline result<T>
err ()
{
  result<T> ret;
  std::memset (&ret, 0, sizeof (ret));
  ret.ok = false;
  return ret;
}

template <typename T>
static inline result<T>
ok (T val)
{
  return result<T>{ val, true };
}

static inline result<void>
ok_void ()
{
  return result<void>{ true };
}
