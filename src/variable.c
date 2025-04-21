#include "variable.h"
#include "dev/errors.h"
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

  page hash_table;
  page hash_leaf;

  err_t ret = pgr_get_expect (&hash_table, PG_HASH_PAGE, 0, v->pgr);
  if (ret)
    {
      return ret;
    }
  u32 idx = fnv1a_hash (vname);

  u64 pgno = hash_table.hp.hashes[idx % *hash_table.hp.len];
  if (pgno == 0)
    {
      // Create new hash page
      ret = pgr_new (&hash_leaf, v->pgr, PG_HASH_LEAF);
      if (ret)
        {
          return ret;
        }
    }
  else
    {
      ret = pgr_get_expect (&hash_leaf, PG_HASH_LEAF, pgno, v->pgr);
      if (ret)
        {
          return ret;
        }
    }

  // TODO - search for key
  *exists = 1;
  return SUCCESS;
}

err_t vr_set (
    var_retriver *v,
    variable *dest,
    const string vname,
    const type *type);
