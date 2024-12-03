#include <assert.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define ASSERT assert
#define MIN(a, b) ((a) > (b) ? (b) : (a))

typedef enum {
  U8 = 0,
  I8 = 1,

  U16 = 2,
  I16 = 3,

  U32 = 4,
  I32 = 4,

  U64 = 5,
  I64 = 6,

  F16 = 7,
  F32 = 8,
  F64 = 9,

  CF16 = 10,
  CF32 = 11,
  CF64 = 12,

  CI8 = 13,
  CI16 = 14,
  CI32 = 15,
  CI64 = 16,
} dtype;

int inttodtype(dtype* dest, int type)
{
  ASSERT(dest);
  if (type < 0)
    return -1;
  if (type > 16)
    return -1;
  *dest = type;
  return 0;
}

#define unreachable() assert(0)

typedef struct {
  size_t start;
  size_t end;
  size_t stride;
  int isinf;
} srange;

#define srange_ASSERT(s)    \
  assert(s.start <= s.end); \
  assert(s.stride > 0)

srange srange_to_srange(srange s)
{
  ASSERT(s.stride == 1);
  return (srange) {
    .start = s.start,
    .end = s.end,
    .isinf = s.isinf,
  };
}

void nsdb_ds_init(FILE* fp, dtype type)
{
  int _type = type;
  fwrite(&_type, sizeof _type, 1, fp);
}

int dtype_fread(dtype* dest, FILE* fp)
{
  ASSERT(dest);
  ASSERT(fp);
  int _type;
  size_t read = fread(&_type, sizeof _type, 1, fp);
  if (read != 1) {
    return -1;
  }
  return inttodtype(dest, _type);
}

///////////////////////////////////////////////
////////////// Circular buffer
typedef struct {
  void* data;
  size_t head;
  size_t tail;
  size_t cap;
  size_t size;
} cbuf;

static inline size_t cbuf_dist(size_t head, size_t tail, size_t cap)
{
  return (head >= tail) ? (head - tail) : (cap - tail + head);
}

static inline size_t cbuf_max_elem(size_t head, size_t tail, size_t cap)
{
  return (head >= tail) ? (cap - head - (tail == 0)) : (tail - head - 1);
}

static inline uint8_t* cbuf_offset(cbuf* buf, size_t idx)
{
  return (uint8_t*)buf->data + idx * buf->size;
}

static inline cbuf cbuf_create(void* data, size_t cap, size_t size)
{
  return (cbuf) { .data = data, .cap = cap, .head = 0, .tail = 0, .size = size };
}

static inline int cbuf_full(cbuf d)
{
  return ((d.head + 1) % d.cap) == d.tail;
}

static inline int cbuf_empty(cbuf d)
{
  return d.head == d.tail;
}

static inline size_t cbuf_len(cbuf d)
{
  return cbuf_dist(d.head, d.tail, d.cap);
}

void cbuf_mv_stride(cbuf* dest, cbuf* src, size_t stride)
{
  if (!stride)
    stride = 1;
  while (!cbuf_full(*dest) && !cbuf_empty(*src)) {
    memcpy(cbuf_offset(dest, dest->head), cbuf_offset(src, src->tail), dest->size);
    dest->head = (dest->head + 1) % dest->cap;

    size_t steps = 0;
    while (steps < stride && !cbuf_empty(*src)) {
      src->tail = (src->tail + 1) % src->cap;
      steps++;
    }

    // Break if src becomes empty
    if (cbuf_empty(*src))
      break;
  }
}

void cbuf_mv(cbuf* dest, cbuf* src)
{
  while (!cbuf_full(*dest) && !cbuf_empty(*src)) {
    memcpy(cbuf_offset(dest, dest->head), cbuf_offset(src, src->tail), dest->size);
    dest->head = (dest->head + 1) % dest->cap;
    src->tail = (src->tail + 1) % src->cap;
  }
}

size_t cbuf_fread(cbuf* d, size_t n, FILE* fp)
{
  size_t total = 0;
  while (total < n && !cbuf_full(*d)) {
    size_t max_elems = cbuf_max_elem(d->head, d->tail, d->cap);
    if (max_elems > n - total)
      max_elems = n - total;
    if (!max_elems)
      break;
    size_t r = fread(cbuf_offset(d, d->head), d->size, max_elems, fp);
    total += r;
    d->head = (d->head + r) % d->cap;
    if (r < max_elems)
      break;
  }
  return total;
}

size_t cbuf_fwrite(cbuf* d, size_t n, FILE* fp)
{
  size_t total = 0;
  while (total < n && !cbuf_empty(*d)) {
    size_t max_elems = cbuf_dist(d->head, d->tail, d->cap);
    if (max_elems > n - total)
      max_elems = n - total;
    if (!max_elems)
      break;
    size_t w = fwrite(cbuf_offset(d, d->tail), d->size, max_elems, fp);
    total += w;
    d->tail = (d->tail + w) % d->cap;
    if (w < max_elems)
      break;
  }
  return total;
}

int nsdb_ds_read_srange(FILE* dest, size_t size, srange s, FILE* src)
{
  // Validate srange
  srange_ASSERT(s);

  // Initialize buffers
  uint8_t _inbuf[2048 * size];
  uint8_t _outbuf[2048 * size];
  cbuf inbuf = cbuf_create(_inbuf, 2048, size);
  cbuf outbuf = cbuf_create(_outbuf, 2048, size);

  // Seek to start position
  size_t k = s.start;
  if (k > 0 && fseek(src, k * size, SEEK_SET)) {
    return -1; // Seek error
  }

  while (1) {
    // Calculate remaining elements
    size_t remaining = s.isinf ? SIZE_MAX : (s.end > k ? s.end - k : 0);
    if (remaining == 0) {
      return 0; // Completed range
    }

    // Read data into inbuf
    size_t toread = MIN(remaining, 2048);
    size_t read = cbuf_fread(&inbuf, toread, src);
    if (read == 0) {
      if (feof(src)) {
        break;
      }
      return -1; // Read error
    }

    k += read;

    // Process data with stride
    while (!cbuf_empty(inbuf)) {
      cbuf_mv_stride(&outbuf, &inbuf, s.stride);

      // Write outbuf to dest
      size_t towrite = cbuf_len(outbuf);
      size_t written = cbuf_fwrite(&outbuf, towrite, dest);
      if (written != towrite) {
        return -1; // Write error
      }

      // Reset outbuf for next batch
      outbuf.tail = outbuf.head;
    }
  }

  return 0;
}

int main()
{
  FILE* dsfile = fopen("foo.txt", "r");
  FILE* ofp = stdout;

  srange s = (srange) {
    .start = 0,
    .end = 5,
    .stride = 5,
    .isinf = 1,
  };

  nsdb_ds_read_srange(ofp, 1, s, dsfile);

  return 0;
}
