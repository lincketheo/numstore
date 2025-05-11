#include "ds/strings.h"  // string
#include "dev/assert.h"  // DEFINE_DBG_ASSERT_I
#include "dev/testing.h" // TEST
#include "intf/stdlib.h" // i_memcmp

DEFINE_DBG_ASSERT_I (string, string, s)
{
  ASSERT (s);
  ASSERT (s->data);
  ASSERT (s->len > 0);
}

DEFINE_DBG_ASSERT_I (string, cstring, s)
{
  string_assert (s);
  ASSERT (s->data[s->len] == 0);
}

string
unsafe_cstrfrom (char *cstr)
{
  return (string){
    .data = cstr,
    .len = i_unsafe_strlen (cstr)
  };
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
  string_assert (&s1);
  string_assert (&s2);
  if (s1.len != s2.len)
    {
      return false;
    }
  return i_strncmp (s1.data, s2.data, s1.len) == 0;
}
