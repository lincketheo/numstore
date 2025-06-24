#include "core/dev/testing.h"  // TODO
#include "core/ds/strings.h"   // TODO
#include "core/errors/error.h" // TODO
#include "core/intf/io.h"      // TODO

#include "server/server.h" // TODO

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
