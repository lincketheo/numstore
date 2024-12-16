#include "numstore/datastruct.hpp"

#include "numstore/errors.hpp"
#include "numstore/facades/stdlib.hpp"
#include "numstore/testing.hpp"
#include "numstore/utils/math.hpp"

extern "C" {
#include <complex.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

///////////////////////////////////////////////////////
///////// bytes

ns_ret_t bytes_create_malloc(bytes *dest, size_t len) {
  ASSERT(dest);
  ASSERT(len > 0);

  void *data;
  ns_ret_t_fallthrough(malloc_f(&data, len));
  dest->data = (byte *)data;
  dest->len = len;
  return NS_OK;
}

ns_ret_t bytes_resize(bytes *b, size_t newlen) {
  ASSERT(b);
  ASSERT(newlen > 0);

  void *_data = (void *)b->data;
  ns_ret_t_fallthrough(realloc_f(&_data, newlen));
  b->data = (byte *)_data;
  b->len = newlen;
  return NS_OK;
}

TEST(bytes_resize) {
  bytes b;
  bytes_create_malloc(&b, 10);
  test_assert_equal(b.len, 10lu, "%lu");
  bytes_resize(&b, 5);
  test_assert_equal(b.len, 5lu, "%lu");
  bytes_resize(&b, 500);
  test_assert_equal(b.len, 500lu, "%lu");
  free_f(b.data);
}

ns_ret_t bytes_resize_and_zero(bytes *b, size_t newlen) {
  ASSERT(b);
  ns_ret_t_fallthrough(bytes_resize(b, newlen));
  if (newlen > b->len) {
    memset(&b->data[b->len], 0, newlen - b->len);
  }
  b->len = newlen;
  return NS_OK;
}

TEST(bytes_resize_and_zero) {
  bytes b;
  bytes_create_malloc(&b, 10);
  test_assert_equal(b.len, 10lu, "%lu");
  bytes_resize_and_zero(&b, 5);
  test_assert_equal(b.len, 5lu, "%lu");
  bytes_resize_and_zero(&b, 500);
  test_assert_equal(b.len, 500lu, "%lu");
  free_f(b.data);
}

ns_ret_t bytes_double_until_space(bytes *b, size_t newcap) {
  ASSERT(b);
  ASSERT(newcap > 0);

  if (newcap > b->len) {
    size_t _newcap = b->len * dblbuf_next_2_power(b->len, newcap);
    bytes_resize(b, _newcap);
  }
  return NS_OK;
}

TEST(bytes_double_until_space) {
  bytes b;
  bytes_create_malloc(&b, 12);
  test_assert_equal(b.len, 12lu, "%lu");
  bytes_double_until_space(&b, 90);
  test_assert_equal(b.len, 96lu, "%lu");
  free_f(b.data);
}

///////////////////////////////////////////////////////
///////// interval
bool intervalf_f_in_exc(intervalf i, float v) {
  ASSERT(i.v0 <= i.vf);
  return v < i.vf && v > i.v0;
}

bool intervalf_if_in_exc(intervalf outer, intervalf inner) {
  ASSERT(outer.v0 <= outer.vf);
  ASSERT(inner.v0 <= inner.vf);

  return intervalf_f_in_exc(outer, inner.v0) &&
         intervalf_f_in_exc(outer, inner.vf);
}

///////////////////////////////////////////////////////
///////// trange
ns_ret_t trange_parse_argstr(trange *dest, const char *str) {
  dest->t0 = 0.0f;
  dest->tf = 0.0f;
  dest->isinf = 0;

  float t0, tf;

  if (sscanf(str, "%f:%f", &t0, &tf) == 2) {
    dest->t0 = t0;
    dest->tf = tf;
  } else if (sscanf(str, "%f:", &t0) == 1) {
    dest->t0 = t0;
    dest->isinf = 1;
  } else if (sscanf(str, ":%f", &tf) == 1) {
    dest->t0 = 0.0f;
    dest->tf = tf;
  } else if (strcmp(str, ":") == 0) {
    dest->isinf = 1;
  } else {
    return NS_EINVAL;
  }

  return NS_OK;
}

TEST(trange_parse_argstr) {
  ns_ret_t res;
  trange t;

  res = trange_parse_argstr(&t, "0:100");
  test_assert_equal(res, NS_OK, "%d");
  test_assert_equal(t.t0, 0.0, "%f");
  test_assert_equal(t.tf, 100.0, "%f");
  test_assert_equal(t.isinf, 0, "%d");

  res = trange_parse_argstr(&t, "0.123:100E8");
  test_assert_equal(res, NS_OK, "%d");
  test_assert_equal(t.t0, 0.123f, "%f");
  test_assert_equal(t.tf, 100E8f, "%f");
  test_assert_equal(t.isinf, 0, "%d");

  res = trange_parse_argstr(&t, "0.123:");
  test_assert_equal(res, NS_OK, "%d");
  test_assert_equal(t.t0, 0.123f, "%f");
  test_assert_equal(t.isinf, 1, "%d");

  res = trange_parse_argstr(&t, ":");
  test_assert_equal(res, NS_OK, "%d");
  test_assert_equal(t.t0, 0.0f, "%f");
  test_assert_equal(t.isinf, 1, "%d");

  res = trange_parse_argstr(&t, ":10");
  test_assert_equal(res, NS_OK, "%d");
  test_assert_equal(t.t0, 0.0f, "%f");
  test_assert_equal(t.tf, 10.0f, "%f");
  test_assert_equal(t.isinf, 0, "%d");
}

///////////////////////////////////////////////////////
///////// frange
bool frange_valid(frange f) {
  if (f.df == 0.0f) {
    return 0;
  }

  if ((f.f1 > f.f0 && f.df <= 0.0f) || (f.f1 < f.f0 && f.df >= 0.0f)) {
    return 0;
  }

  return 1;
}

bool frange_in_range(frange f, float fs) {
  intervalf fsint = (intervalf){
      .v0 = -fs / 2,
      .vf = fs / 2,
  };

  return intervalf_if_in_exc(fsint, frange_to_intervalf(f));
}

ns_ret_t frange_parse_argstr(frange *dest, const char *str) {
  ASSERT(dest);
  ASSERT(str);

  float f0 = 0.0f, f1 = 0.0f, df = 0.0f;

  if (sscanf(str, "%f:%f:%f", &f0, &df, &f1) != 3) {
    return NS_EINVAL;
  }

  dest->f0 = f0;
  dest->f1 = f1;
  dest->df = df;

  if (!frange_valid(*dest))
    return NS_EINVAL;

  return NS_OK;
}

TEST(frange_parse_argstr) {
  frange result;
  ns_ret_t ret;

  // Valid cases
  ret = frange_parse_argstr(&result, "0.0:0.1:1.0");
  test_assert_equal(ret, NS_OK, "%d");
  test_assert_equal(result.f0, 0.0f, "%f");
  test_assert_equal(result.df, 0.1f, "%f");
  test_assert_equal(result.f1, 1.0f, "%f");

  ret = frange_parse_argstr(&result, "1.5:-0.5:0.0");
  test_assert_equal(ret, NS_OK, "%d");
  test_assert_equal(result.f0, 1.5f, "%f");
  test_assert_equal(result.df, -0.5f, "%f");
  test_assert_equal(result.f1, 0.0f, "%f");

  // Invalid cases
  ret = frange_parse_argstr(&result, "0.0:-0.1:1.0");
  test_assert_equal(ret, NS_EINVAL, "%d");

  ret = frange_parse_argstr(&result, "1.0:0.1:0.0");
  test_assert_equal(ret, NS_EINVAL, "%d");

  ret = frange_parse_argstr(&result, "5:0:-5");
  test_assert_equal(ret, NS_EINVAL, "%d");

  ret = frange_parse_argstr(&result, "not_a_number");
  test_assert_equal(ret, NS_EINVAL, "%d");

  ret = frange_parse_argstr(&result, "1.0:1.0");
  test_assert_equal(ret, NS_EINVAL, "%d");
}

size_t frange_size(frange f) { return (size_t)floorf((f.f1 - f.f0) / (f.df)); }

void frange_fprintf(FILE *ofp, frange f) {
  fprintf(ofp, "frange:\n");
  fprintf(ofp, "  f0 (start frequency): %.2f\n", f.f0);
  fprintf(ofp, "  f1 (end frequency): %.2f\n", f.f1);
  fprintf(ofp, "  df (frequency step): %.2f\n", f.df);
}

intervalf frange_to_intervalf(frange f) {
  if (f.f1 > f.f0) {
    return (intervalf){
        .v0 = f.f0,
        .vf = f.f1,
    };
  }

  return (intervalf){
      .v0 = f.f1,
      .vf = f.f0,
  };
}

///////////////////////////////////////////////////////
///////// krange
ns_ret_t krange_parse_argstr(krange *dest, const char *str) {
  dest->k0 = 0.0f;
  dest->kf = 0.0f;
  dest->isinf = 0;

  int k0, kf;

  if (sscanf(str, "%d:%d", &k0, &kf) == 2) {
    dest->k0 = k0;
    dest->kf = kf;
  } else if (sscanf(str, "%d:", &k0) == 1) {
    dest->k0 = k0;
    dest->isinf = 1;
  } else if (sscanf(str, ":%d", &kf) == 1) {
    dest->k0 = 0.0f;
    dest->kf = kf;
  } else if (strcmp(str, ":") == 0) {
    dest->isinf = 1;
  } else {
    return NS_EINVAL;
  }

  return NS_OK;
}

TEST(krange_parse_argstr) {
  ns_ret_t res;
  krange t;

  res = krange_parse_argstr(&t, "0:100");
  test_assert_equal(res, 0, "%d");
  test_assert_equal(t.k0, 0, "%d");
  test_assert_equal(t.kf, 100, "%d");
  test_assert_equal(t.isinf, 0, "%d");

  res = krange_parse_argstr(&t, "0:100");
  test_assert_equal(res, 0, "%d");
  test_assert_equal(t.k0, 0, "%d");
  test_assert_equal(t.kf, 100, "%d");
  test_assert_equal(t.isinf, 0, "%d");

  res = krange_parse_argstr(&t, "123:");
  test_assert_equal(res, 0, "%d");
  test_assert_equal(t.k0, 123, "%d");
  test_assert_equal(t.isinf, 1, "%d");

  res = krange_parse_argstr(&t, ":");
  test_assert_equal(res, 0, "%d");
  test_assert_equal(t.k0, 0, "%d");
  test_assert_equal(t.isinf, 1, "%d");

  res = krange_parse_argstr(&t, ":10");
  test_assert_equal(res, 0, "%d");
  test_assert_equal(t.k0, 0, "%d");
  test_assert_equal(t.kf, 10, "%d");
  test_assert_equal(t.isinf, 0, "%d");
}

krange trange_to_krange(trange t, float fs) {
  krange ret = {.k0 = (int)(fs * t.t0), .isinf = t.isinf};
  if (!ret.isinf) {
    ret.kf = (int)(fs * t.tf);
  }
  return ret;
}

finite_krange krange_to_finite_krange(krange k) {
  ASSERT(!k.isinf);
  return (finite_krange){
      .k0 = k.k0,
      .kf = k.kf,
  };
}

void krange_fprintf(FILE *ofp, krange k) {
  fprintf(ofp, "krange:\n");
  fprintf(ofp, "  k0 (start sample): %d\n", k.k0);

  if (k.isinf) {
    fprintf(ofp, "  kf (end sample): infinity\n");
  } else {
    fprintf(ofp, "  kf (end sample): %d\n", k.kf);
  }

  fprintf(ofp, "  isinf: %s\n", k.isinf ? "true" : "false");
}

krange_next_result krange_next(krange k, size_t desired) {
  if (k.isinf)
    return (krange_next_result){
        .val = desired,
        .next = k,
    };

  size_t kavail = krange_size(k);
  if (desired > kavail) {
    return (krange_next_result){
        .val = kavail,
        .next =
            (krange){
                .k0 = k.kf,
                .kf = k.kf,
                .isinf = k.isinf,
            },
    };
  }

  return (krange_next_result){
      .val = desired,
      .next =
          (krange){
              .k0 = (int)((size_t)k.k0 + desired),
              .kf = k.kf,
              .isinf = k.isinf,
          },
  };
}

size_t krange_size(krange k) {
  if (k.isinf)
    return SIZE_MAX;
  ASSERT(k.kf >= k.k0);
  return k.kf - k.k0;
}

int krange_more(krange k) { return k.isinf || k.kf > k.k0; }

///////////////////////////////////////////////////////
///////// buffers
cf64_buffer cf64_buffer_from_bytes(bytes b) {
  ASSERT(b.len % sizeof(cf64) == 0);
  cf64_buffer ret;
  ret.len = b.len / sizeof(cf64);
  ret.data = (cf64 *)b.data;
  return ret;
}

shift_buf shift_buf_create(void *data, size_t size, size_t cap) {
  shift_buf buf;
  buf.data = data;
  buf.size = size;
  buf.cap = cap;
  buf.len = 0;
  return buf;
}

ns_ret_t shift_buf_fread_max(shift_buf *dest, FILE *ifp) {
  ASSERT(dest->len <= dest->cap);

  if (dest->len == dest->cap)
    return NS_OK;

  size_t space = dest->cap - dest->len;
  size_t read = space;
  void *buf = (void *)((char *)dest->data + dest->len * dest->size);
  ns_ret_t_fallthrough(fread_f(buf, dest->size, &read, ifp));

  dest->len += read;
  return NS_OK;
}

void shift_buf_del_front(shift_buf *dest, size_t n) {
  ASSERT(dest);
  ASSERT(n > 0);
  ASSERT(dest->len <= dest->cap);

  if (n > dest->len)
    n = dest->len;

  void *src = (void *)((char *)dest->data + n * dest->size);
  memmove(dest->data, src, (dest->len - n) * dest->size);

  dest->len -= n;
}

///////////////////////////////////////////////////////
///////// pargs
ns_ret_t pargs_parse_argstr(pargs *dest, const char *str) {
  ASSERT(str);

  dest->a = 1.0f;
  dest->p0 = 0.0f;
  dest->f = NAN;

  const char *ptr = str;

  while (*ptr) {
    char *end;
    float value = strtof(ptr, &end);
    if (end == ptr) {
      return NS_EINVAL;
    }

    if (*end == 'f') {
      dest->f = value;
      ptr = end + 1;
    } else if (*end == 'a') {
      dest->a = value;
      ptr = end + 1;
    } else if (*end == 'p') {
      dest->p0 = value;
      ptr = end + 1;
    } else {
      ptr = end;
    }
  }

  if (isnanf(dest->f)) {
    return NS_EINVAL;
  }

  return NS_OK;
}

TEST(pargs_parse_argstr) {
  int res;
  pargs p;

  // Base
  res = pargs_parse_argstr(&p, "5f4p1a");
  test_assert_equal(res, NS_OK, "%d");
  test_assert_equal(p.a, 1.0, "%f");
  test_assert_equal(p.p0, 4.0, "%f");
  test_assert_equal(p.f, 5.0, "%f");

  // Order doesn't matter
  res = pargs_parse_argstr(&p, "4.123p1E9f1a");
  test_assert_equal(res, NS_OK, "%d");
  test_assert_equal(p.a, 1.0f, "%f");
  test_assert_equal(p.p0, 4.123f, "%f");
  test_assert_equal(p.f, 1e9f, "%f");

  // Leave out a
  res = pargs_parse_argstr(&p, "1e9f4.123p");
  test_assert_equal(res, NS_OK, "%d");
  test_assert_equal(p.a, 1.0f, "%f");
  test_assert_equal(p.p0, 4.123f, "%f");
  test_assert_equal(p.f, 1e9f, "%f");

  // Leave out p
  res = pargs_parse_argstr(&p, "-1.100f9.1a");
  test_assert_equal(res, NS_OK, "%d");
  test_assert_equal(p.a, 9.1f, "%f");
  test_assert_equal(p.p0, 0.0f, "%f");
  test_assert_equal(p.f, -1.1f, "%f");

  // Leave out p and a
  res = pargs_parse_argstr(&p, "5.123f");
  test_assert_equal(res, NS_OK, "%d");
  test_assert_equal(p.a, 1.0f, "%f");
  test_assert_equal(p.p0, 0.0f, "%f");
  test_assert_equal(p.f, 5.123f, "%f");

  // Error
  res = pargs_parse_argstr(&p, "");
  test_assert_equal(res, NS_EINVAL, "%d");

  res = pargs_parse_argstr(&p, "gf");
  test_assert_equal(res, NS_EINVAL, "%d");

  res = pargs_parse_argstr(&p, "5fgp");
  test_assert_equal(res, NS_EINVAL, "%d");

  res = pargs_parse_argstr(&p, "5f6p5i");
  test_assert_equal(res, NS_EINVAL, "%d");
}

cf64 pargs_evalk(pargs p, int k, float fs) {
  float arg = (2.0f * M_PI * p.f * (float)k / fs + p.p0);
  cf64 c = cexpf(I * arg);
  float ff = cabsf(c);
  return c * p.a;
}

///////////////////////////////////////////////////////
///////// si_prefix
static si_prefixf prefixesf[] = {
    {"y", 1e-24}, {"z", 1e-21}, {"a", 1e-18}, {"f", 1e-15}, {"p", 1e-12},
    {"n", 1e-9},  {"u", 1e-6},  {"m", 1e-3},  {"", 1.0},    {"k", 1e3},
    {"M", 1e6},   {"G", 1e9},   {"T", 1e12},  {"P", 1e15},  {"E", 1e18},
    {"Z", 1e21},  {"Y", 1e24}};

static const int si_prefix_count = sizeof(prefixesf) / sizeof(si_prefixf);

ns_ret_t si_prefixf_parse(si_prefixf *dest, const char *prefix) {
  for (int i = 0; i < si_prefix_count; i++) {
    if (strcmp(prefixesf[i].prefix, prefix) == 0) {
      *dest = prefixesf[i];
      return NS_OK;
    }
  }
  return NS_EINVAL;
}

static si_prefix prefixes[] = {
    {"y", 1e-24}, {"z", 1e-21}, {"a", 1e-18}, {"f", 1e-15}, {"p", 1e-12},
    {"n", 1e-9},  {"u", 1e-6},  {"m", 1e-3},  {"", 1.0},    {"k", 1e3},
    {"M", 1e6},   {"G", 1e9},   {"T", 1e12},  {"P", 1e15},  {"E", 1e18},
    {"Z", 1e21},  {"Y", 1e24}};

ns_ret_t si_prefix_parse(si_prefix *dest, const char *prefix) {
  for (int i = 0; i < si_prefix_count; i++) {
    if (strcmp(prefixes[i].prefix, prefix) == 0) {
      *dest = prefixes[i];
      return NS_OK;
    }
  }
  return NS_EINVAL;
}

///////////////////////////////////////////////////////
///////// slice
slice slice_default() {
  return (slice){
      .start = 0,
      .step = 1,
      .isinf = 1,
  };
}

size_t slice_left(slice s) {
  if (s.isinf) {
    return SIZE_MAX;
  }
  ASSERT(s.stop >= s.start);
  return s.stop - s.start;
}

ns_ret_t slice_parse(slice *dest, const char *str) {
  ASSERT(dest);
  ASSERT(str);

  *dest = slice_default();

  size_t start, stop, step;

  if (strchr(str, ':') == NULL) {
    return NS_OK;
  }

  if (sscanf(str, "%zu:%zu:%zu", &start, &stop, &step) == 3) {
    dest->start = start;
    dest->stop = stop;
    dest->step = step;
    dest->isinf = 0;
  } else if (sscanf(str, "%zu:%zu:", &start, &stop) == 2) {
    dest->start = start;
    dest->stop = stop;
    dest->step = 1;
    dest->isinf = 0;
  } else if (sscanf(str, ":%zu:%zu", &stop, &step) == 2) {
    dest->start = 0;
    dest->stop = stop;
    dest->step = step;
    dest->isinf = 0;
  } else if (sscanf(str, "%zu::%zu", &start, &step) == 2) {
    dest->start = start;
    dest->step = step;
    dest->isinf = 1;
  } else if (sscanf(str, ":%zu", &stop) == 1) {
    dest->start = 0;
    dest->stop = stop;
    dest->step = 1;
    dest->isinf = 0;
  } else if (sscanf(str, "%zu:", &start) == 1) {
    dest->start = start;
    dest->step = 1;
    dest->isinf = 1;
  } else if (strcmp(str, "::") == 0) {
    dest->start = 0;
    dest->step = 1;
    dest->isinf = 1;
  } else {
    return NS_EINVAL;
  }

  if (dest->start > dest->stop) {
    return NS_EINVAL;
  }

  return NS_OK;
}

///////////////////////////////////////////////////////
///////// stack
ns_ret_t stack_create_malloc(stack *dest, size_t initial_cap) {
  ASSERT(initial_cap > 0);
  void *_data;
  ns_ret_t_fallthrough(malloc_f(&_data, initial_cap));
  dest->data = (byte *)_data;
  dest->dcap = initial_cap;
  dest->head = 0;
  return NS_OK;
}

stack stack_create_from(uint8_t *data, size_t initial_cap) {
  ASSERT(initial_cap > 0);
  stack ret;
  ret.data = data;
  ret.dcap = initial_cap;
  ret.head = 0;
  return ret;
}

size_t stack_cap_left(stack *s) {
  ASSERT(s);
  ASSERT(s->head <= s->dcap);
  return s->dcap - s->head;
}

static inline ns_ret_t stack_ensure_space(stack *s, size_t next_slen) {
  bytes b = (bytes){.data = s->data, .len = s->dcap};
  ns_ret_t_fallthrough(bytes_double_until_space(&b, s->head + next_slen));
  s->data = b.data;
  s->dcap = b.len;
  return NS_OK;
}

ns_ret_t stack_push_grow(stack *dest, void *src, size_t slen) {
  ASSERT(dest);
  ASSERT(src);
  ASSERT(slen > 0);
  ASSERT(dest->dcap > 0);
  ASSERT(dest->data);

  ns_ret_t_fallthrough(stack_ensure_space(dest, slen));
  memcpy(dest->data + dest->head, src, slen);
  dest->head += slen;
  return NS_OK;
}

TEST(stack_push_stackalloc) {
  stack b;
  stack_create_malloc(&b, 10);
  int d = 1;
  stack_push_grow(&b, &d, sizeof(int));
  d = 2;
  stack_push_grow(&b, &d, sizeof(int));
  d = 10;
  stack_push_grow(&b, &d, sizeof(int));
  d = 123;
  stack_push_grow(&b, &d, sizeof(int));
  d = 5;
  stack_push_grow(&b, &d, sizeof(int));

  int *dd = (int *)b.data;
  test_assert_equal(dd[0], 1, "%d");
  test_assert_equal(dd[1], 2, "%d");
  test_assert_equal(dd[2], 10, "%d");
  test_assert_equal(dd[3], 123, "%d");
  test_assert_equal(dd[4], 5, "%d");

  free_f(b.data);
}

void stack_push_dont_grow(stack *dest, void *src, size_t slen) {
  ASSERT(dest);
  ASSERT(src);
  ASSERT(slen > 0);
  ASSERT(dest->dcap > 0);
  ASSERT(dest->data);

  if (dest->dcap < dest->head + slen) {
    fatal_error("Stack overflow\n");
    unreachable();
  }

  memcpy(dest->data + dest->head, src, slen);
  dest->head += slen;
}

bytes stacktobytes(stack s) {
  return (bytes){
      .data = s.data,
      .len = s.head,
  };
}

ns_ret_t stacktobytes_clip(bytes *dest, stack s) {
  bytes ret = stacktobytes(s);
  ns_ret_t_fallthrough(bytes_resize(&ret, s.head));
  *dest = ret;
  return NS_OK;
}

TEST(stacktobytes_clip) {
  stack b;
  stack_create_malloc(&b, 10);
  int d = 1;
  stack_push_grow(&b, &d, sizeof(int));
  d = 2;
  stack_push_grow(&b, &d, sizeof(int));
  d = 10;
  stack_push_grow(&b, &d, sizeof(int));
  d = 123;
  stack_push_grow(&b, &d, sizeof(int));
  d = 5;
  stack_push_grow(&b, &d, sizeof(int));

  bytes bb;
  stacktobytes_clip(&bb, b);
  test_assert_equal(bb.len, 5 * sizeof(int), "%lu");

  int *dd = (int *)bb.data;
  test_assert_equal(dd[0], 1, "%d");
  test_assert_equal(dd[1], 2, "%d");
  test_assert_equal(dd[2], 10, "%d");
  test_assert_equal(dd[3], 123, "%d");
  test_assert_equal(dd[4], 5, "%d");

  free_f(bb.data);
}
}
