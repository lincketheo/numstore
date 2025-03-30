#include "fdbtree.h"
#include "_assert.h"
#include "bnode.h"

#include <assert.h>
#include <stdio.h>
#include <unistd.h>

static inline child_ptr_t fdbtree_get_root(fdbtree* b)
{
  lseek(b->fd, 0, SEEK_SET);
  child_ptr_t root;
  ssize_t r = read(b->fd, &root, sizeof(root));
  assert(r == sizeof(root));
  return root;
}

static bnode fdbtree_fetch_node(fdbtree* b, child_ptr_t ptr)
{
  bnode ret;
  ssize_t read = pread(b->fd, &ret, PAGE_SIZE, PAGE_SIZE * ptr);
  assert(read == PAGE_SIZE);
  return ret;
}

static child_ptr_t fdbtree_append_node(fdbtree* b, const bnode* node)
{
  // Check position
  off_t pos = lseek(b->fd, 0, SEEK_END);
  todo_assert(pos > 0);
  assert(pos % PAGE_SIZE == 0);
  child_ptr_t page = pos / PAGE_SIZE;

  // Do write
  ssize_t written = write(b->fd, node, PAGE_SIZE);
  assert(written == PAGE_SIZE);

  return page;
}

// TODO - I think this is the bad one for database Atomicity
static void fdbtree_update_node(fdbtree* b, const bnode* node, child_ptr_t loc) {
  bnode_assert(node);
  fdbtree_assert(b);

  ssize_t written = pwrite(b->fd, node, PAGE_SIZE, PAGE_SIZE * loc);
  assert(written == PAGE_SIZE);
}

static void fdbtree_insert_recurs(
    fdbtree* b, bnode_kv* k,
    child_ptr_t root, bnode* pass_up,
    int* should_pass_up)
{
  bnode_kv_assert(k);

  bnode node = fdbtree_fetch_node(b, root);

  // Child Node
  if (bnode_is_child(&node)) {

    // Insert the key value into the node,
    // This might overflow, but bnode is
    // intentionally 2 * PAGE_SIZE
    // Afterwards, we'll split
    bnode newnode;
    int ret = bnode_insert_kv(&newnode, &node, k);
    assert(ret == 0);

    // Split
    if (bnode_size(&newnode) > PAGE_SIZE) {
      // BNode Axiom 1
      // If bnode is full, then it must have at least 3
      // (because non full has at least 2)
      assert(newnode.nkeys > 2);

      // Split into three parts
      bnode left;
      bnode right;
      bnode_split_part_1(&left, pass_up, &right, &newnode);
      *should_pass_up = 1;

      child_ptr_t left_ptr = fdbtree_append_node(b, &left);
      child_ptr_t right_ptr = fdbtree_append_node(b, &right);

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

  child_ptr_t child = bnode_ptrs(&node)[idx];
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
      child_ptr_t left_ptr = fdbtree_store_node(b, &temp2);
      child_ptr_t right_ptr = fdbtree_store_node(b, &temp3);
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
  child_ptr_t root = fdbtree_root(b);
}

static int fdbtree_fetch_recurs(fdbtree* b, bnode_kv* k, child_ptr_t root)
{
  bnode_kv_assert(k);

  bnode node = fdbtree_fetch_node(b, root);
  child_ptr_t idx;
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
