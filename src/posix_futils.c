#include "posix_futils.h"

#include "common_assert.h"
#include "common_string.h" // path_join
#include "common_testing.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

bool
file_exists_any_type (const char *_fname)
{
  struct stat buf;
  return (stat (_fname, &buf) == 0);
}

bool
dir_exists (const char *dname)
{
  struct stat buf;
  if (stat (dname, &buf))
    {
      return false;
    }

  return S_ISDIR (buf.st_mode);
}

bool
file_exists (const char *dname)
{
  struct stat buf;
  if (stat (dname, &buf))
    {
      return false;
    }

  return S_ISREG (buf.st_mode);
}

fexists_ret
file_exists_in_dir (linmem *m, const char *dname, const char *_fname)
{

  char full_name[2048];
  if (path_join (full_name, 2048, dname, _fname))
    {
      return FER_ARGS_TOO_LONG;
    }

  if (file_exists (full_name))
    return FER_EXISTS;

  return FER_DOESNT_EXIST;
}

#ifndef TEST
int
test_file_exists_in_dir ()
{
  TEST_EQUAL (file_exists_in_dir ("foo", "bar"), FER_DOESNT_EXIST, "%d");

  TEST_EQUAL (mkdir ("foo", 0755), 0, "%d");
  TEST_EQUAL (file_exists_in_dir ("foo", "bar"), FER_DOESNT_EXIST, "%d");

  FILE *fp = fopen ("foo/bar", "w+");
  TEST_EQUAL (fp == NULL, 0, "%d");
  fclose (fp);

  TEST_EQUAL (file_exists_in_dir ("foo", "bar"), FER_EXISTS, "%d");

  TEST_EQUAL (remove ("foo/bar"), 0, "%d");
  TEST_EQUAL (remove ("foo"), 0, "%d");

  TEST_EQUAL (file_exists_in_dir ("foo", "bar"), FER_DOESNT_EXIST, "%d");

  return 0;
}
REGISTER_TEST (test_file_exists_in_dir);
#endif

int
create_dir (const char *dirname, int mode)
{
  errno = 0;
  if (mkdir (dirname, mode))
    {
      return errno;
    }
  return 0;
}

fcreate_ret
create_file_in_dir (const char *dname, const char *fname)
{
  // Create full path name
  char full_name[2048];
  if (path_join (full_name, 2048, dname, fname))
    {
      return (fcreate_ret){
        .type = FCR_ARGS_TOO_LONG,
      };
    }

  // Check if it already exists
  if (file_exists (full_name))
    {
      return (fcreate_ret){
        .type = FCR_ALREADY_EXISTS,
      };
    }

  // Create the file
  errno = 0;
  FILE *fp = fopen (full_name, "w+");
  if (fp == NULL)
    {
      return (fcreate_ret){
        .type = FCR_OS_FAILURE,
        .errno_if_os_failure = errno,
      };
    }

  errno = 0;
  if (fclose (fp))
    {
      return (fcreate_ret){
        .type = FCR_OS_FAILURE,
        .errno_if_os_failure = errno,
      };
    }

  return (fcreate_ret){
    .type = FCR_SUCCESS,
  };
}
