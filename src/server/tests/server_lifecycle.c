#include "dev/testing.h"
#include "ds/strings.h"
#include "errors/error.h"
#include "intf/io.h"
#include "server/server.h"

TEST (server_open_green_path)
{
  error e = error_create (NULL);
  test_fail_if (i_remove_quiet (unsafe_cstrfrom ("foo.db"), &e));

  // Create a green path server - should be not null
  server *s = server_open (8129, unsafe_cstrfrom ("foo.db"), &e);
  test_fail_if_null (s);

  // Close server
  test_fail_if (server_close (s, &e));
}

TEST (server_open_duplicate_ports)
{
  error e = error_create (NULL);
  test_fail_if (i_remove_quiet (unsafe_cstrfrom ("foo.db"), &e));

  // Create a green path server - should be not null
  server *s1 = server_open (8129, unsafe_cstrfrom ("foo.db"), &e);
  test_fail_if_null (s1);

  // Create a server on the same port
  server *s2 = server_open (8129, unsafe_cstrfrom ("foo.db"), &e);
  test_assert_equal (s2, NULL);
  test_assert_equal (e.cause_code, ERR_IO);
  e.cause_code = SUCCESS;

  // Close server
  test_fail_if (server_close (s1, &e));
}
