#include "impl/fbtree.h"

void fbtree_insert(btree* _b, const bnode_kv k)
{
  fbtree* b = (fbtree*)_b;
  b->fd;
}

static int fbtree_fetch(btree* b, bnode_kv* k)
{
}

fbtree fbtree_create(bnode_kv k0, int fd)
{
  fbtree ret;
  ret.tree.insert = fbtree_insert;
  ret.tree.fetch = fbtree_fetch;
}
