#include "utils/hashing.h"

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
