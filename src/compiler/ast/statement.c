#include "compiler/ast/statement.h"
#include "core/intf/io.h"

static const char *TAG = "Statement";

statement *
statement_create (error *e)
{
  statement *ret = i_malloc (1, sizeof *ret);
  if (ret == NULL)
    {
      error_causef (e, ERR_NOMEM, "%s not enough memory for statement", TAG);
      return NULL;
    }

  ret->qspace = lalloc_create_from (ret->query_space);
  return ret;
}

void
statement_free (statement *s)
{
  i_free (s);
}
