#include "ast/type/types.h"
#include "compiler/tokens.h"
#include "errors/error.h"
#include "intf/io.h"

#include <stddef.h> // size_t

typedef struct
{
  lalloc *work; // Where to allocate work
  lalloc *dest; // Where to allocate result
  u32 tnum;     // Token number
  error *e;     // Return status
} parser_ctxt;

static inline parser_ctxt __attribute__ ((unused))
pctx_create (lalloc *work, lalloc *dest)
{
  return (parser_ctxt){
    .work = work,
    .tnum = 0,
    .dest = dest,
    .e = NULL,
  };
}

void *lemon_parseAlloc (void *(*mallocProc) (size_t));

void lemon_parseFree (void *p, void (*freeProc) (void *));

void lemon_parse (void *yyp, int yymajor, token yyminor, parser_ctxt *ctxt);

#define parse_alloc() lemon_parseAlloc (malloc)
#define parse_free(p) lemon_parseFree (p, free)

static inline err_t
parse (void *yyp, token tok, parser_ctxt *ctxt, error *e)
{
  ctxt->e = e;
  lemon_parse (yyp, tok.type, tok, ctxt);
  err_t ret = err_t_from (ctxt->e);
  if (!ret)
    {
      ctxt->tnum++;
    }
  return ret;
}
