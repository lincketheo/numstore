#include "common_string.h"

#include "common_testing.h"

int
path_join (char *dest, size_t dlen, const char *prefix, const char *suffix)
{
  ASSERT (dest);
  ASSERT (prefix);
  ASSERT (suffix);

  size_t plen = strnlen (prefix, dlen);
  size_t slen = strnlen (suffix, dlen);

  if (plen + slen >= dlen - 2)
    {
      return -1;
    }

  memcpy (dest, prefix, plen);

  // Remove trailing slashes
  while (plen > 0 && dest[plen - 1] == '/')
    {
      plen--;
    }
  dest[plen++] = '/';

  // Skip leading slashes
  while (*suffix == '/')
    {
      suffix++;
      slen--;
    }

  memcpy (&(dest[plen]), suffix, slen);
  dest[plen + slen] = '\0';

  return 0;
}

#ifndef TEST
int
test_path_join ()
{
  char ret[2048];

  TEST_EQUAL (path_join (ret, 2048, "foo", "bar"), 0, "%d");
  TEST_ASSERT_STR_EQUAL (ret, "foo/bar");
  TEST_EQUAL (path_join (ret, 2048, "foo/", "bar"), 0, "%d");
  TEST_ASSERT_STR_EQUAL (ret, "foo/bar");
  TEST_EQUAL (path_join (ret, 2048, "foo//", "bar"), 0, "%d");
  TEST_ASSERT_STR_EQUAL (ret, "foo/bar");
  TEST_EQUAL (path_join (ret, 2048, "./foo//", "bar"), 0, "%d");
  TEST_ASSERT_STR_EQUAL (ret, "./foo/bar");
  TEST_EQUAL (path_join (ret, 2048, "./foo//", "./bar"), 0, "%d");
  TEST_ASSERT_STR_EQUAL (ret, "./foo/./bar");
  TEST_EQUAL (path_join (ret, 2048, "./foo//", "/bar"), 0, "%d");
  TEST_ASSERT_STR_EQUAL (ret, "./foo/bar");
  TEST_EQUAL (path_join (ret, 2048, "./foo//", "//bar"), 0, "%d");
  TEST_ASSERT_STR_EQUAL (ret, "./foo/bar");

  char buff[3000];
  memset (buff, 0, sizeof (buff));
  memset (buff, 'a', sizeof (buff));
  buff[sizeof (buff) - 1] = '\0';
  TEST_EQUAL (path_join (ret, 2048, buff, "//bar"), -1, "%d");

  return 0;
}
REGISTER_TEST (test_path_join);
#endif
