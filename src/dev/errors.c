#include "dev/errors.h"
#include "dev/assert.h"

const char *
err_t_to_str (err_t e)
{
  switch (e)
    {
    case SUCCESS:
      {
        return "SUCCESS";
      }
    case ERR_IO:
      {
        return "ERR_IO";
      }
    case ERR_INVALID_STATE:
      {
        return "ERR_INVALID_STATE";
      }
    case ERR_ALREADY_EXISTS:
      {
        return "ERR_ALREADY_EXISTS";
      }
    case ERR_DOESNT_EXIST:
      {
        return "case ERR_DOESNT_EXIST";
      }
    case ERR_NOMEM:
      {
        return "case ERR_NOMEM";
      }
    case ERR_PGSTACK_OVERFLOW:
      {
        return "case ERR_PGSTACK_OVERFLOW";
      }
    case ERR_ARITH:
      {
        return "case ERR_ARITH";
      }
    case ERR_NOT_LOADED:
      {
        return "case ERR_NOT_LOADED";
      }
    case ERR_INVALID_ARGUMENT:
      {
        return "case ERR_INVALID_ARGUMENT";
      }
    case ERR_EMPTY:
      {
        return "case ERR_EMPTY";
      }
    case ERR_FALLBACK:
      {
        return "case ERR_FALLBACK";
      }
    }
  panic ();
  return "";
}
