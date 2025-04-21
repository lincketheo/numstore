#include "variable.h"
#include "dev/errors.h"
#include "intf/stdlib.h"
#include "paging.h"
#include "sds.h"
#include "typing.h"

////////////////////////////// Hash Function

u32
fnv1a_hash (const string s)
{
  u32 hash = 2166136261u;
  char *str = s.data;
  for (u32 i = 0; i < s.len; ++i)
    {
      hash ^= (u8)*str++;
      hash *= 16777619u;
    }
  return hash;
}

////////////////////////////// A Single Variable

DEFINE_DBG_ASSERT_I (variable, variable, v)
{
  ASSERT (v);
  ASSERT (v->type);
  // TODO - more
}

////////////////////////////// A Subset of a Variable

DEFINE_DBG_ASSERT_I (variable_subset, variable_subset, v)
{
  ASSERT (v);
  type_subset_assert (v->type_subset);
  variable_assert (&v->v);
}

////////////////////////////// A Hash Map of Variables

DEFINE_DBG_ASSERT_H (var_retriver, var_retriver, v)
{
  ASSERT (v);
  pager_assert (v->pgr);
}

err_t
vr_get (
    var_retriver *v,
    variable *dest,
    int *exists,
    const string vname)
{
  variable_assert (dest);
  string_assert (&vname);

  page hash_page;
  page hash_leaf;

  // Fetch the hash table node
  // NOTE - what if hash table is more than 1 page?
  err_t_wrap (pgr_get_expect (&hash_page, PG_HASH_PAGE, 0, v->pgr));
  u32 idx = fnv1a_hash (vname);

  u64 pgno = hash_page.hp.hashes[idx % *hash_page.hp.len];
  if (pgno == 0)
    {
      // Doesn't exist (note 0 denotes doesn't exist)
      // Create new hash page
      err_t_wrap (pgr_new (&hash_leaf, v->pgr, PG_HASH_LEAF));
    }

  // Iterate through hash leafs
  do
    {
      // Fetch the hash_leaf
      err_t_wrap (pgr_get_expect (&hash_leaf, PG_HASH_LEAF, pgno, v->pgr));

      hash_leaf_tuple hlt;
      for (u32 i = 0; i < *hash_leaf.hl.nvalues; ++i)
        {
          err_t_wrap (hl_get_tuple (&hlt, &hash_leaf, 0));

          if (string_equal (
                  vname,
                  (string){
                      .data = hlt.str,
                      .len = *hlt.strlen,
                  }))
            {
              *exists = 1;
              dest->type = 0; // TODO
              dest->page0 = *hlt.pg0;
              return SUCCESS;
            }
        }

      // Fetch next hash leaf page
      pgno = *hash_leaf.hl.next;
    }
  while (pgno != 0);

  // TODO - search for key
  *exists = 0;
  return SUCCESS;
}

err_t
vr_set (
    var_retriver *v,
    variable *dest,
    const string vname,
    const type *type);
