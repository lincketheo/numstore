#include "fdbtree.h"
#include "bnode.h"
#include "errors.h"
#include "ns_assert.h"
#include "page_alloc.h"
#include "types.h"

#include <assert.h>
#include <unistd.h>

int fdbtree_open(fdbtree* dest, bnode_kv k0, const char* fname)
{
  bnode_kv_assert(&k0);
  assert(dest);

  // Build page alloc
  page_alloc p;
  int stat = page_alloc_open(&p, fname);
  if (stat != SUCCESS) {
    return stat;
  }

  dest->alloc = p;
  fdbtree_assert(dest);
  return SUCCESS;
}

// Returns the first node
// Which is just the first page
static inline page_ptr fdbtree_get_root(fdbtree* b)
{
  fdbtree_assert(b);
  u8 buffer[PAGE_SIZE];
  int ret = page_alloc_get(&b->alloc, buffer, 0);
  if (ret != SUCCESS) {
    return ret;
  }
  page_ptr root = *((page_ptr*)&buffer[0]);
  return root;
}

static int fdbtree_insert_recurs(
    fdbtree* b, bnode_kv* k,
    page_ptr root, bnode* pass_up,
    int* should_pass_up)
{
  bnode_kv_assert(k);

  bnode node;
  int ret = page_alloc_get(&b->alloc, (u8*)&node, root);
  if (ret != SUCCESS) {
    return ret;
  }

  // Child Node
  if (bnode_is_leaf(&node)) {

    // Insert the key value into the node,
    // This might overflow, but bnode is
    // intentionally 2 * PAGE_SIZE
    // Afterwards, we'll split
    bnode newnode;
    ret = bnode_insert_kv(&newnode, &node, k);
    if (ret != SUCCESS) {
      return ret;
    }

    // Split
    if (bnode_size(&newnode) > PAGE_SIZE) {
      // BNode Axiom 1
      // If bnode is full, then it must have at least 3
      // (because non full has at least 2)
      assert(newnode.nkeys > 2);

      // Split into three parts
      bnode left;
      bnode right;
      bnode center;
      bnode_split_part_1(&left, &center, &right, &newnode);

      page_ptr left_ptr = fdbtree_append_node(b, &left);
      page_ptr right_ptr = fdbtree_append_node(b, &right);

      bnode_split_part_2(left_ptr, pass_up, right_ptr);

    } else {
      fdbtree_write_node(b, root, &newnode);
      *should_pass_up = 0;
    }
    return;
  }

  // Inner Node
  u64 idx;
  if (bnode_find_kv(&node, k, &idx)) {
    int ret = bnode_insert_kv(&temp1, &node, k);
    assert(ret == 0);
    fdbtree_write_node(b, root, &temp1);
    *should_pass_up = 0;
    return;
  }

  page_ptr child = bnode_ptrs(&node)[idx];
  bnode child_pass;
  int child_should_pass = 0;
  fdbtree_insert_recurs(b, k, child, &child_pass, &child_should_pass);

  if (child_should_pass) {
    int ret = bnode_insert_kv(&temp1, &node, &child_pass);
    assert(ret == 0);

    if (bnode_size(&temp1) > PAGE_SIZE) {
      assert(temp1.nkeys > 0);
      bnode_split(&temp2, pass_up, &temp3, &temp1);
      *should_pass_up = 1;
      page_ptr left_ptr = fdbtree_store_node(b, &temp2);
      page_ptr right_ptr = fdbtree_store_node(b, &temp3);
      bnode_ptrs(pass_up)[0] = left_ptr;
      bnode_ptrs(pass_up)[1] = right_ptr;
    } else {
      fdbtree_write_node(b, root, &temp1);
      *should_pass_up = 0;
    }
  } else {
    fdbtree_write_node(b, root, &node);
    *should_pass_up = 0;
  }
}
}

static void fdbtree_insert(fdbtree* b, const bnode_kv k)
{
  page_ptr root = fdbtree_root(b);
}

static int fdbtree_fetch_recurs(fdbtree* b, bnode_kv* k, page_ptr root)
{
  bnode_kv_assert(k);

  bnode node = fdbtree_fetch_node(b, root);
  page_ptr idx;
  if (bnode_find_kv(&node, k, &idx)) {
    return 1;
  }
  if (k->ptr == 0) {
    return 0;
  }

  return fdbtree_fetch_recurs(b, k, k->ptr);
}

static int fdbtree_fetch(fdbtree* b, bnode_kv* k)
{
  assert(b);
  assert(k);
  return fdbtree_fetch_recurs(b, k, fdbtree_root(b));
}
