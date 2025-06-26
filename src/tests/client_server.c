#include "core/dev/testing.h"  // TODO
#include "core/ds/strings.h"   // TODO
#include "core/errors/error.h" // TODO
#include "core/intf/io.h"      // TODO
#include "core/intf/logging.h" // TODO

#include "client/client.h" // TODO

#include "server/server.h" // TODO

/**
void
__test_client_server (char *cmnd, u32 niters)
{
  error e = error_create (NULL);

  server *s = server_open (8123, unsafe_cstrfrom ("test.db"), &e);
  test_fail_if_null (s);
  client *c = client_open ("127.0.0.1", 8123, &e);
  test_fail_if_null (c);

  client_send (c, unsafe_cstrfrom (cmnd), &e);

  // Accept
  for (u32 i = 0; i < niters; ++i)
    {
      i_log_error ("ITER: %d\n", i);
      server_execute (s);
    }

  string resp = client_recv (c, &e);
  test_fail_if (e.cause_code);

  test_assert_equal (string_equal (unsafe_cstrfrom ("OK"), resp), true);

  test_fail_if (server_close (s, &e));
  test_fail_if (client_close (c, &e));
  i_free (resp.data);
}

TEST (client_server)
{
  __test_client_server ("create a u32;", 4);
  __test_client_server ("create b struct { i i32, b u32 };", 6);
  __test_client_server ("create c struct { i i32, b u32, c f16 };", 7);
  __test_client_server ("create d union { i i32, b u32 };", 6);
  __test_client_server ("create e enum { foo, bar };", 5);
  __test_client_server ("create f [10][9][8] i32;", 5);
  __test_client_server ("create g union{i enum{foo, bar,     biz } , c struct { i i32, asdf [10][9][8] union { a i32, b u32 }, b cf128, d i8, e    f16 } };", 16);
}
*/
