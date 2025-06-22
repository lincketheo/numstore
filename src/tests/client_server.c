#include "client/client.h"
#include "dev/testing.h"
#include "ds/strings.h"
#include "errors/error.h"
#include "intf/logging.h"
#include "server/server.h"

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
}

TEST (client_server)
{
  __test_client_server ("create a u32;", 4);
  __test_client_server ("create a struct { i i32, b u32 };", 6);
  __test_client_server ("create a struct { i i32, b u32, c f16 };", 7);
  __test_client_server ("create a union { i i32, b u32 };", 6);
  __test_client_server ("create a enum { foo, bar };", 5);
  __test_client_server ("create a [10][9][8] i32;", 5);
  __test_client_server ("create a union{i enum{foo, bar,     biz } , c struct { i i32, asdf [10][9][8] union { a i32, b u32 }, b cf128, d i8, e    f16 } };", 16);
}
