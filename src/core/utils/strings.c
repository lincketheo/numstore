#include "core/utils/strings.h"

#include "core/dev/assert.h" // TODO
#include "core/dev/testing.h"
#include "core/errors/error.h"
#include "core/intf/stdlib.h" // TODO
#include "core/mm/lalloc.h"

u64
line_length (const char *buf, u64 max)
{
  ASSERT (buf);
  ASSERT (max > 0);

  char *nl = i_memchr (buf, '\n', max);
  u64 ret;

  if (nl != NULL)
    {
      ret = (u64)(nl - buf);
    }
  else
    {
      ret = max;
    }

  return ret;
}

int
strings_all_unique (const string *strs, u32 count)
{
  for (u32 i = 0; i < count; ++i)
    {
      for (u32 j = i + 1; j < count; ++j)
        {
          if (strs[i].len != strs[j].len)
            {
              continue;
            }
          if (i_memcmp (strs[i].data, strs[j].data, strs[i].len) == 0)
            {
              return 0;
            }
        }
    }
  return 1;
}

TEST (strings_all_unique)
{
  // empty array: trivially unique
  test_assert_int_equal (strings_all_unique (NULL, 0), 1);

  // one string: unique
  char data1[] = "hello";
  string s1[] = { { 5, data1 } };
  test_assert_int_equal (strings_all_unique (s1, 1), 1);

  // two different strings, different lengths
  char d1[] = "a";
  char d2[] = "ab";
  string s2[] = { { 1, d1 }, { 2, d2 } };
  test_assert_int_equal (strings_all_unique (s2, 2), 1);

  // two different strings, same length
  char e1[] = "ab";
  char e2[] = "cd";
  string s3[] = { { 2, e1 }, { 2, e2 } };
  test_assert_int_equal (strings_all_unique (s3, 2), 1);

  // duplicate strings
  char f1[] = "dup";
  char f2[] = "dup";
  string s4[] = { { 3, f1 }, { 3, f2 } };
  test_assert_int_equal (strings_all_unique (s4, 2), 0);

  // multiple strings with one duplicate in the middle
  char g1[] = "one";
  char g2[] = "two";
  char g3[] = "one";
  char g4[] = "four";
  string s5[] = { { 3, g1 }, { 3, g2 }, { 3, g3 }, { 4, g4 } };
  test_assert_int_equal (strings_all_unique (s5, 4), 0);

  // all unique in larger set
  char h1[] = "aa";
  char h2[] = "bb";
  char h3[] = "cc";
  char h4[] = "dd";
  string s6[] = { { 2, h1 }, { 2, h2 }, { 2, h3 }, { 2, h4 } };
  test_assert_int_equal (strings_all_unique (s6, 4), 1);
}

bool
string_equal (const string s1, const string s2)
{
  if (s1.len != s2.len)
    {
      return false;
    }
  return i_strncmp (s1.data, s2.data, s1.len) == 0;
}

string
string_plus (const string left, const string right, lalloc *alloc, error *e)
{
  u32 len = left.len + right.len;
  ASSERT (len > 0); // TODO - this might be allowable

  char *data = lmalloc (alloc, len, 1);

  if (data == NULL)
    {
      error_causef (e, ERR_NOMEM, "Failed to allocate %d bytes for string concat", len);
      return (string){
        .data = NULL,
        .len = 0,
      };
    }

  i_memcpy (data, left.data, left.len);
  i_memcpy (data + left.len, right.data, right.len);

  return (string){
    .data = data,
    .len = len,
  };
}

const string *
strings_are_disjoint (
    const string *left, u32 llen,
    const string *right, u32 rlen)
{
  for (u32 i = 0; i < llen; ++i)
    {
      for (u32 j = 0; j < rlen; ++j)
        {
          if (string_equal (left[i], right[j]))
            {
              return &left[i];
            }
        }
    }

  return NULL;
}

bool
string_less_string (const string left, const string right)
{
  u32 min_len = (left.len < right.len) ? left.len : right.len;
  int cmp = i_memcmp (left.data, right.data, min_len);
  if (cmp < 0)
    {
      return true;
    }
  if (cmp > 0)
    {
      return false;
    }
  return left.len < right.len;
}

bool
string_greater_string (const string left, const string right)
{
  u32 min_len = (left.len < right.len) ? left.len : right.len;
  int cmp = i_memcmp (left.data, right.data, min_len);
  if (cmp > 0)
    {
      return true;
    }
  if (cmp < 0)
    {
      return false;
    }
  return left.len > right.len;
}

bool
string_less_equal_string (const string left, const string right)
{
  return !string_greater_string (left, right);
}

bool
string_greater_equal_string (const string left, const string right)
{
  return !string_less_string (left, right);
}
