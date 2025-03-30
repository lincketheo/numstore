#include "bnode.h"
#include "testing.h"
#include "types.h"
#include "utils.h"

//////////////////////////////////// A Key Value Data Structure

u64 bnode_kv_size(const bnode_kv* b)
{
  bnode_kv_assert(b);
  return b->keylen + sizeof(data_ptr_t) + sizeof(keylen_t);
}

static inline void bnode_kv_serialize(u8* _dest, bnode_kv* k)
{
  bnode_kv_assert(k);
  u8* dest = _dest;

  // First, the key length
  memcpy(dest, &k->keylen, sizeof(keylen_t));
  dest += sizeof(keylen_t);

  // Then the key
  memcpy(dest, k->key, k->keylen);
  dest += k->keylen;

  // Then the "value" (data pointer)
  memcpy(dest, &k->ptr, sizeof(data_ptr_t));
}

//////////////////////////////////// A B-Node Data Structure

static inline child_ptr_t* bnode_ptrs(const bnode* b)
{
  return (child_ptr_t*)b->data;
}

static inline offset_t* bnode_offsets(const bnode* b)
{
  child_ptr_t* child_ptrs_tail = bnode_ptrs(b) + (b->nkeys + 1);
  return (offset_t*)child_ptrs_tail;
}

static inline u8* bnode_keys(const bnode* b)
{
  offset_t* offsets_tail = bnode_offsets(b) + b->nkeys;
  return (u8*)offsets_tail;
}

u64 bnode_size(const bnode* b)
{
  // Last offset is the size
  offset_t offset = bnode_offsets(b)[b->nkeys - 1];
  u64 before = bnode_keys(b) - (u8*)b;

  u64 size = before + offset;
  assert(size <= PAGE_SIZE);

  return size;
}

static void bnode_copy_range(
    bnode* dest,
    const bnode* src,
    int d0, int s0, int n)
{
  assert(n > 0);

  child_ptr_t* cdest = bnode_ptrs(dest);
  offset_t* odest = bnode_offsets(dest);

  const child_ptr_t* csrc = bnode_ptrs(src);
  const offset_t* osrc = bnode_offsets(src);

  u8* dest_keys = bnode_keys(dest);
  const u8* src_keys = bnode_keys(src);

  offset_t dest_base = (d0 == 0) ? 0 : odest[d0 - 1];
  offset_t src_base = (s0 == 0) ? 0 : osrc[s0 - 1];

  offset_t src_end = osrc[s0 + n - 1];
  offset_t bytes_to_copy = src_end - src_base;

  memcpy(dest_keys + dest_base, src_keys + src_base, bytes_to_copy);
  memcpy(cdest + d0, csrc + s0, (n + 1) * sizeof(child_ptr_t));

  for (int i = 0; i < n; i++) {
    odest[d0 + i] = dest_base + (osrc[s0 + i] - src_base);
  }
}

nkeys_t bnode_get_median_key(const bnode* node)
{
  assert(node->nkeys >= 2);
  nkeys_t nleft = node->nkeys / 2;

#define bleft (sizeof(nkeys_t) + nleft * (sizeof(child_ptr_t) + sizeof(offset_t)) + bnode_offsets(node)[nleft])

  while (bleft > PAGE_SIZE) {
    nleft--;
  }
  assert(nleft >= 1);

#define bright (bnode_size(node) - bleft + 4)

  while (bright > PAGE_SIZE) {
    nleft++;
  }
  assert(nleft < node->nkeys);

#undef bleft
#undef bright

  return nleft;
}

void bnode_split_part_1(
    bnode* left,
    bnode* center,
    bnode* right,
    const bnode* src)
{
  nkeys_t start = bnode_get_median_key(src);
}

void bnode_split_part_2(
    child_ptr_t left,
    bnode* center,
    child_ptr_t right)
{
}

int bnode_is_child(const bnode* b)
{
  bnode_assert(b);

  // Proof:
  // Knuth criterion 5: "A non leaf node with
  // k children contains k - 1 keys"
  // Also, all nodes have at least 1 key
  // A Leaf node at [0] is obviously 0, but
  // a non leaf node is [0] is not 0 because
  // all non leaf nodes have
  // at least 2 children, so not 0
  return bnode_ptrs(b)[0] == 0;
}

static inline u8* bnode_kv_start(const bnode* b, u64 idx)
{
  bnode_assert(b);
  assert(idx < b->nkeys);

  if (idx == 0) {
    return bnode_keys(b);
  }

  offset_t* offsets = bnode_offsets(b);
  return bnode_keys(b) + offsets[idx - 1];
}

static inline bnode_kv bnode_get_kv(const bnode* b, u64 idx)
{
  assert(b);
  assert(idx < b->nkeys);

  bnode_kv ret;

  u8* head = bnode_kv_start(b, idx);
  ret.keylen = *(keylen_t*)(head);
  ret.key = (char*)(head + sizeof(keylen_t));
  ret.ptr = *(data_ptr_t*)(head + sizeof(keylen_t) + ret.keylen);

  bnode_kv_assert(&ret);

  return ret;
}

static inline void bnode_set_kv(const bnode* b, u64 idx, data_ptr_t ptr)
{
  assert(b);
  assert(idx < b->nkeys);

  bnode_kv kv;

  u8* head = bnode_kv_start(b, idx);
  kv.keylen = *(keylen_t*)(head);
  kv.key = (char*)(head + sizeof(keylen_t));
  *(data_ptr_t*)(head + sizeof(keylen_t) + kv.keylen) = ptr;

  bnode_kv_assert(&kv);
}

int bnode_find_kv(const bnode* b, bnode_kv* k, u64* idx)
{
  assert(b);
  assert(k);
  assert(idx);

  // Binary Search
  int left = 0;
  int right = b->nkeys - 1;

  while (left <= right) {
    int mid = left + (right - left) / 2;

    bnode_kv _k = bnode_get_kv(b, mid);
    keylen_t klen = MIN(_k.keylen, k->keylen);
    int cmp = strncmp(_k.key, k->key, klen);

    if (cmp == 0) {
      if (_k.keylen == k->keylen) {
        *idx = mid;
        k->ptr = _k.ptr;
        return 1;
      } else if (k->keylen < _k.keylen) {
        right = mid - 1;
      } else {
        left = mid + 1;
      }
    } else if (cmp < 0) {
      left = mid + 1;
    } else {
      right = mid - 1;
    }
  }

  *idx = left;
  return 0;
}

static inline void bnode_kv_print(FILE* ofp, bnode_kv* b)
{
  assert(ofp);
  bnode_kv_assert(b);

  fprintf(ofp, "(%.*s, %" PRIu64 ")", b->keylen, b->key, b->ptr);
}

static bnode bnode_create(bnode_kv k0)
{
  bnode ret;
  ret.nkeys = 1;

  child_ptr_t* ptrs = bnode_ptrs(&ret);
  ptrs[0] = 0;
  ptrs[1] = 0;

  offset_t* offsets = bnode_offsets(&ret);
  offsets[0] = bnode_kv_size(&k0);

  u8* keys = bnode_keys(&ret);
  bnode_kv_serialize(keys, &k0);

  return ret;
}

__attribute__((unused)) static void bnode_print(FILE* ofp, bnode* b)
{
  bnode_assert(b);

  child_ptr_t* ptrs = bnode_ptrs(b);

  // Print Nodes
  fprintf(ofp, "%" PRIu64 " ", ptrs[0]);
  for (int i = 0; i < b->nkeys; ++i) {
    bnode_kv key = bnode_get_kv(b, i);
    bnode_kv_print(ofp, &key);
    fprintf(ofp, " %" PRIu64 " ", ptrs[i + 1]);
  }
  fprintf(ofp, "\n");
}

__attribute__((unused)) static void bnode_byte_print(FILE* ofp, bnode* b)
{
  bnode_assert(b);

  fprintf(ofp, "NKeys: %" PRIu16 "\n", b->nkeys);
  fprintf(ofp, "Node Size: %" PRIu64 "\n", bnode_size(b));
  fprintf(ofp, "Data:\n");
  pretty_print_bytes(ofp, (u8*)b, bnode_size(b));
}

int bnode_insert_kv(bnode* dest, const bnode* src, bnode_kv* k)
{
  bnode_assert(src);
  bnode_kv_assert(k);

  // First, search for the key or location the key should be
  u64 idx;
  bnode_kv found_kv;
  memcpy(&found_kv, k, sizeof *k);
  int found = bnode_find_kv(src, &found_kv, &idx);
  if (found) {
    memcpy(dest, src, bnode_size(src));
    bnode_set_kv(dest, idx, k->ptr);
    return 0;
  }

  dest->nkeys = src->nkeys + 1;

  child_ptr_t* dest_ptrs = bnode_ptrs(dest);
  u8* dest_keys = bnode_keys(dest);
  offset_t* dest_offsets = bnode_offsets(dest);

  const child_ptr_t* src_ptrs = bnode_ptrs(src);
  const u8* src_keys = bnode_keys(src);
  const offset_t* src_offsets = bnode_offsets(src);

  // Length of the keys byte array on the left of [idx]
  offset_t kvlen_left = (idx > 0) ? src_offsets[idx - 1] : 0;

  if (idx > 0) {
    memcpy(dest_ptrs, src_ptrs, idx * sizeof(*dest_ptrs));
    memcpy(dest_offsets, src_offsets, idx * sizeof(*dest_offsets));
    memcpy(dest_keys, src_keys, kvlen_left * sizeof(*dest_keys));
  }

  // Third, dest[idx] = kv
  u64 kvlen = bnode_kv_size(k);
  bnode_kv_serialize(dest_keys + kvlen_left, k);
  dest_offsets[idx] = kvlen_left + kvlen;
  dest_ptrs[idx] = 0;

  // Fourth, dest[idx + 1:nkeys + 1] = src[idx:nkeys]
  // Don't forget to add the size of the newly inserted key to
  // all the offsets
  u64 nleft = src->nkeys - idx;
  if (nleft > 0) {
    // Copy Pointers Over
    memcpy(&dest_ptrs[idx + 1], &src_ptrs[idx],
        (nleft + 1) * sizeof(*dest_ptrs));

    // Copy Offsets Over
    memcpy(&dest_offsets[idx + 1], &src_offsets[idx],
        nleft * sizeof(*dest_offsets));

    // Add new keylen to all offsets
    for (u64 i = idx + 1; i < dest->nkeys; i++) {
      dest_offsets[i] += kvlen;
    }

    // Copy keys over
    offset_t kvlen_right = src_offsets[src->nkeys - 1] - kvlen_left;
    memcpy(dest_keys + dest_offsets[idx], src_keys + kvlen_left,
        kvlen_right * sizeof(*dest_keys));
  }

  return 0;
}

// Some testing utils
#define BNODE_KV_FROM(cstr, _ptr) \
  ((bnode_kv) { .keylen = strlen(cstr), .key = (char*)(cstr), .ptr = (_ptr) })

#define test_assert_bnode_kv_equal(_k0, _k1)                          \
  do {                                                                \
    test_assert_equal((_k0).keylen, (_k1).keylen, "%" PRIu16);        \
    test_assert_equal(strncmp((_k0).key, (_k1).key, (_k0).keylen), 0, \
        "%" PRId32);                                                  \
    test_assert_equal((_k0).ptr, (_k1).ptr, "%" PRIu64);              \
  } while (0)

TEST(insertion)
{
  bnode_kv k0 = BNODE_KV_FROM("foobar", 1);
  bnode_kv k1 = BNODE_KV_FROM("fizb", 2);
  bnode_kv k2 = BNODE_KV_FROM("bazbi", 3);
  bnode_kv k3 = BNODE_KV_FROM("barbuz", 4);
  bnode_kv k4 = BNODE_KV_FROM("bar", 5);
  bnode_kv k5 = BNODE_KV_FROM("barbuzbiz", 6);

  // duplicate
  bnode_kv k6 = BNODE_KV_FROM("bazbi", 9);

  bnode b0 = bnode_create(k0);
  bnode b1;
  test_assert_equal(bnode_insert_kv(&b1, &b0, &k1), 1, "%" PRId32);
  test_assert_equal(bnode_insert_kv(&b0, &b1, &k2), 1, "%" PRId32);
  test_assert_equal(bnode_insert_kv(&b1, &b0, &k3), 1, "%" PRId32);
  test_assert_equal(bnode_insert_kv(&b0, &b1, &k4), 1, "%" PRId32);
  test_assert_equal(bnode_insert_kv(&b1, &b0, &k5), 1, "%" PRId32);
  test_assert_equal(bnode_insert_kv(&b0, &b1, &k6), 0, "%" PRId32);

  test_assert_equal(b0.nkeys, 6, "%" PRId32);

  bnode_kv r0 = bnode_get_kv(&b0, 0);
  bnode_kv r1 = bnode_get_kv(&b0, 1);
  bnode_kv r2 = bnode_get_kv(&b0, 2);
  bnode_kv r3 = bnode_get_kv(&b0, 3);
  bnode_kv r4 = bnode_get_kv(&b0, 4);
  bnode_kv r5 = bnode_get_kv(&b0, 5);

  bnode_kv exp0 = BNODE_KV_FROM("bar", 5);
  bnode_kv exp1 = BNODE_KV_FROM("barbuz", 4);
  bnode_kv exp2 = BNODE_KV_FROM("barbuzbiz", 6);
  bnode_kv exp3 = BNODE_KV_FROM("bazbi", 9);
  bnode_kv exp4 = BNODE_KV_FROM("fizb", 2);
  bnode_kv exp5 = BNODE_KV_FROM("foobar", 1);

  test_assert_bnode_kv_equal(r0, exp0);
  test_assert_bnode_kv_equal(r1, exp1);
  test_assert_bnode_kv_equal(r2, exp2);
  test_assert_bnode_kv_equal(r3, exp3);
  test_assert_bnode_kv_equal(r4, exp4);
  test_assert_bnode_kv_equal(r5, exp5);
}

TEST(find)
{
  bnode_kv k0 = BNODE_KV_FROM("foobar", 1);
  bnode_kv k1 = BNODE_KV_FROM("fizb", 2);
  bnode_kv k2 = BNODE_KV_FROM("bazbi", 3);
  bnode_kv k3 = BNODE_KV_FROM("barbuz", 4);

  bnode b0 = bnode_create(k0);
  bnode b1;
  test_assert_equal(bnode_insert_kv(&b1, &b0, &k1), 1, "%" PRId32);
  test_assert_equal(bnode_insert_kv(&b0, &b1, &k2), 1, "%" PRId32);
  test_assert_equal(bnode_insert_kv(&b1, &b0, &k3), 1, "%" PRId32);

  u64 idx;
  int found;
  bnode_kv search;

  search = BNODE_KV_FROM("foobar", 0);
  found = bnode_find_kv(&b1, &search, &idx);
  test_assert_equal(found, 1, "%" PRId32);
  test_assert_equal(idx, (u64)3, "%" PRIu64);
  test_assert_equal(search.ptr, (u64)1, "%" PRIu64);

  search = BNODE_KV_FROM("bar", 0);
  found = bnode_find_kv(&b1, &search, &idx);
  test_assert_equal(found, 0, "%" PRId32);
  test_assert_equal(idx, (u64)0, "%" PRIu64);

  search = BNODE_KV_FROM("barbuzbiz", 0);
  found = bnode_find_kv(&b1, &search, &idx);
  test_assert_equal(found, 0, "%" PRId32);
  test_assert_equal(idx, (u64)1, "%" PRIu64);

  search = BNODE_KV_FROM("zzzz", 0);
  found = bnode_find_kv(&b1, &search, &idx);
  test_assert_equal(found, 0, "%" PRId32);
  test_assert_equal(idx, (u64)4, "%" PRIu64);
}
