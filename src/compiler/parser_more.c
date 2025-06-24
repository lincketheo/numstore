#include <stdio.h>  // TODO
#include <stdlib.h> // TODO

#include "core/errors/error.h" // TODO
#include "core/mm/lalloc.h"    // TODO

#include "compiler/parser.h" // TODO

extern void *lemon_parseAlloc (void *(*mallocProc) (size_t));

extern void lemon_parseFree (
    void *p,
    void (*freeProc) (void *));

extern void lemon_parse (
    void *yyp,
    int yymajor,
    token yyminor,
    parser_result *ctxt);

extern void lemon_parseTrace (FILE *out, char *zPrefix);

static const char *TAG = "Parser";

err_t
parser_create (parser_result *ret, lalloc *work, lalloc *dest, error *e)
{
  ASSERT (ret);

  void *yyp = lemon_parseAlloc (malloc);
  if (yyp == NULL)
    {
      return error_causef (e, ERR_NOMEM, "%s Failed to allocate lemon parser", TAG);
    }

  *ret = (parser_result){
    .yyp = yyp,
    .work = work,
    .dest = dest,
    .tnum = 0,
    .e = NULL,
    .ready = false,
  };

  return SUCCESS;
}

query
parser_consume (parser_result *p)
{
  ASSERT (p->ready);
  query ret = p->result;
  lemon_parseFree (p->yyp, free);

  p->tnum = 0;
  p->e = NULL;
  p->ready = false;

  return ret;
}

err_t
parser_parse (parser_result *res, token tok, error *e)
{
  res->e = e;
  lemon_parse (res->yyp, tok.type, tok, res);
  err_t ret = err_t_from (res->e);
  if (!ret)
    {
      res->tnum++;
    }
  return ret;
}
