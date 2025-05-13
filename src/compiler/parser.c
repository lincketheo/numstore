#include "compiler/parser.h"

#include "compiler/stack_parser/common.h"
#include "compiler/stack_parser/queries/create.h"
#include "compiler/stack_parser/queries/delete.h"
#include "compiler/stack_parser/types/enum.h"
#include "compiler/stack_parser/types/kvt.h"
#include "compiler/stack_parser/types/sarray.h"
#include "compiler/tokens.h"
#include "dev/assert.h" // DEFINE_DBG_ASSERT_I
#include "errors/error.h"
#include "mm/lalloc.h"
#include "type/types.h"

DEFINE_DBG_ASSERT_I (parser, parser, p)
{
  ASSERT (p);
  ASSERT (p->tokens_input);
  ASSERT (p->queries_output);
}

typedef struct
{
  sb_build_type type;

  union
  {
    type t;
    query q;
  };
} ast_result;

//////////////////////////////////////////// UTILS

static inline ast_parser
parser_pop (parser *sp)
{
  parser_assert (sp);
  ASSERT (sp->sp > 0);
  return sp->stack[--sp->sp];
}

static inline ast_parser *
parser_top (parser *sp)
{
  parser_assert (sp);
  ASSERT (sp->sp > 0);
  return &sp->stack[sp->sp - 1];
}

static inline void
parser_push_expect (parser *p, ast_parser value)
{
  parser_assert (p);
  ASSERT (p->sp < 20);
  p->stack[p->sp++] = value;
}

static inline err_t
parser_push (parser *p, ast_parser value, error *e)
{
  parser_assert (p);

  if (p->sp == 20)
    {
      return error_causef (
          e, ERR_NOMEM,
          "Parser: stack overflow");
    }

  parser_push_expect (p, value);
  return SUCCESS;
}

/**
 * Given your current state and the next token [t], what
 * do you expect next?
 */
static inline sb_feed_t
ast_parser_expect_next (ast_parser *b, token t)
{
  switch (b->type)
    {
    case SBBT_TYPE:
      {
        switch (b->tb.state)
          {
          case TB_UNKNOWN:
            {
              return SBFT_TOKEN;
            }
          case TB_STRUCT:
          case TB_UNION:
            {
              return b->tb.kvp.state == KVTP_WAITING_FOR_TYPE ? SBFT_TYPE : SBFT_TOKEN;
            }
          case TB_ENUM:
            {
              // Enum always expects token
              return SBFT_TOKEN;
            }
          case TB_SARRAY:
            {
              if (b->tb.sap.state == SAP_WAITING_FOR_LEFT_OR_TYPE && t.type == TT_LEFT_BRACKET)
                {
                  return SBFT_TOKEN;
                }
              else
                {
                  return SBFT_TYPE;
                }
            }
          default:
            {
              UNREACHABLE ();
            }
          }
      }
    case SBBT_QUERY:
      {
        query_parser *qb = &b->qb;
        switch (qb->state)
          {
          case QP_UNKNOWN:
            {
              return SBFT_TOKEN;
            }
          case QP_CREATE:
            {
              return qb->cp.state == CB_WAITING_FOR_TYPE ? SBFT_TYPE : SBFT_TOKEN;
            }
          case QP_DELETE:
            {
              return SBFT_TOKEN;
            }
          }
        return SBFT_TOKEN;
      }
    default:
      {
        UNREACHABLE ();
      }
    }
}

static inline stackp_result
ast_parser_accept_token (ast_parser *b, token t, lalloc *alloc, error *e)
{
  switch (b->type)
    {
    case SBBT_TYPE:
      {
        switch (b->tb.state)
          {
          case TB_UNKNOWN:
            {
              switch (t.type)
                {
                case TT_STRUCT:
                  {
                    b->tb.state = TB_STRUCT;
                    b->tb.kvp = kvp_create (alloc);
                    return SPR_CONTINUE;
                  }
                case TT_UNION:
                  {
                    b->tb.state = TB_UNION;
                    b->tb.kvp = kvp_create (alloc);
                    return SPR_CONTINUE;
                  }
                case TT_ENUM:
                  {
                    b->tb.state = TB_ENUM;
                    b->tb.enp = enp_create (alloc);
                    return SPR_CONTINUE;
                  }
                case TT_LEFT_BRACKET:
                  {
                    b->tb.state = TB_SARRAY;
                    b->tb.sap = sap_create (alloc);
                    return SPR_CONTINUE;
                  }
                case TT_PRIM:
                  {
                    b->tb.ret = (type){ .type = T_PRIM, .p = t.prim };
                    b->tb.state = TB_PRIM;
                    return SPR_DONE;
                  }
                default:
                  {
                    return SPR_SYNTAX_ERROR;
                  }
                }
            }
          case TB_STRUCT:
          case TB_UNION:
            {
              return kvp_accept_token (&b->tb.kvp, t, e);
            }
          case TB_ENUM:
            {
              return enp_accept_token (&b->tb.enp, t, e);
            }
          case TB_SARRAY:
            {
              return sap_accept_token (&b->tb.sap, t, e);
            }
          default:
            {
              return SPR_SYNTAX_ERROR;
            }
          }
      }
    case SBBT_QUERY:
      {
        switch (b->qb.state)
          {
          case QP_UNKNOWN:
            {
              switch (t.type)
                {
                case TT_CREATE:
                  {
                    b->qb.state = QP_CREATE;
                    b->qb.cp = crtp_create ();
                    return SPR_CONTINUE;
                  }
                case TT_DELETE:
                  {
                    b->qb.state = QP_CREATE;
                    b->qb.dp = dltp_create ();
                    return SPR_CONTINUE;
                  }
                default:
                  {
                    return SPR_SYNTAX_ERROR;
                  }
                }
            }
          case QP_CREATE:
            {
              return crtp_accept_token (&b->qb.cp, t, e);
            }
          case QP_DELETE:
            {
              return dltp_accept_token (&b->qb.dp, t, e);
            }
          default:
            {
              return SPR_SYNTAX_ERROR;
            }
          }
      }
    default:
      {
        UNREACHABLE ();
      }
    }
}

static inline stackp_result
ast_parser_accept_type (ast_parser *b, type t, error *e)
{
  switch (b->type)
    {
    case SBBT_TYPE:
      {
        switch (b->tb.state)
          {
          case TB_STRUCT:
          case TB_UNION:
            {
              return kvp_accept_type (&b->tb.kvp, t, e);
            }
          case TB_SARRAY:
            {
              return sap_accept_type (&b->tb.sap, t, e);
            }
          default:
            {
              return SPR_SYNTAX_ERROR;
            }
          }
      }
    case SBBT_QUERY:
      {
        switch (b->qb.state)
          {
          case QP_CREATE:
            {
              return crtp_accept_type (&b->qb.cp, t, e);
            }
          default:
            {
              return SPR_SYNTAX_ERROR;
            }
          }
      }
    default:
      {
        UNREACHABLE ();
      }
    }
}

static inline stackp_result
ast_parser_build (ast_parser *b, lalloc *destination, error *e)
{
  switch (b->type)
    {
    case SBBT_TYPE:
      {
        switch (b->tb.state)
          {
          case TB_STRUCT:
            {
              return kvp_build_struct (&b->tb.ret.st, &b->tb.kvp, destination, e);
            }
          case TB_UNION:
            {
              return kvp_build_union (&b->tb.ret.un, &b->tb.kvp, destination, e);
            }
          case TB_ENUM:
            {
              return enp_build (&b->tb.ret.en, &b->tb.enp, destination, e);
            }
          case TB_SARRAY:
            {
              return sap_build (&b->tb.ret.sa, &b->tb.sap, destination, e);
            }
          case TB_PRIM:
            {
              return SPR_DONE;
            }
          default:
            {
              return SPR_SYNTAX_ERROR;
            }
          }
      }
    case SBBT_QUERY:
      {
        switch (b->qb.state)
          {
          case QP_CREATE:
            {
              return crtp_build (&b->qb.ret.cquery, &b->qb.cp, e);
            }
          case QP_DELETE:
            {
              return dltp_build (&b->qb.ret.dquery, &b->qb.dp, e);
            }
          default:
            {
              return SPR_SYNTAX_ERROR;
            }
          }
      }
    default:
      {
        UNREACHABLE ();
      }
    }
}

stackp_result
parser_feed_token (parser *tp, token t, error *e)
{
  parser_assert (tp);
  ASSERT (tp->sp > 0);

  /**
   * First, get the top ast parser from the
   * stack - and ask: "Hey parser, do you
   * want a token or a type next?"
   */
  ast_parser *top = parser_top (tp);
  sb_feed_t exp = ast_parser_expect_next (top, t);

  /**
   * Top Parser wants a type, we push a new
   * type builder onto the top of the stack and
   * by the way, all type parsers need to accept
   * a token first
   */
  if (exp == SBFT_TYPE)
    {
      ast_parser _next = {
        .tb = (type_parser){
            .state = TB_UNKNOWN,
        },
        .type = SBBT_TYPE,
      };

      err_t ret = parser_push (tp, _next, e);
      if (ret)
        {
          return (stackp_result)ret;
        }

      top = parser_top (tp);

      ASSERT (ast_parser_expect_next (top, t) == SBFT_TOKEN);
    }

  /**
   * Give the top ast parser a token
   */
  stackp_result ret = ast_parser_accept_token (top, t, &tp->working_space, e);
  if (ret < 0)
    {
      return ret;
    }

  /**
   * If we just finished the top parser,
   * we loop and build each item on the
   * stack then feed it to the parser
   * beneath it
   *
   * sp == 1 is done because we've reached the base
   */
  while (tp->sp > 1 && ret == SPR_DONE)
    {
      // Pop the top off the stack
      ast_parser top = parser_pop (tp);

      /**
       * And call it's build routine
       */
      ret = ast_parser_build (&top, &tp->ctrl->types_alloc, e);
      if (ret < 0)
        {
          return ret;
        }

      /**
       * Done returns either DONE or ERROR, not CONTINUE
       */
      ASSERT (ret == SPR_DONE);

      /**
       * Merge it into the next element
       */
      ret = ast_parser_accept_type (&tp->stack[tp->sp - 1], top.tb.ret, e);
      if (ret < 0)
        {
          return ret;
        }
      continue;
    }

  return ret;
}

void
parser_begin (parser *sp)
{
  parser_assert (sp);
  ASSERT (sp->sp == 0);

  ast_parser top = {
    .qb = (query_parser){
        .state = QP_UNKNOWN,
    },
    .type = SBBT_QUERY,
  };
  parser_push_expect (sp, top);
}

/**
 * Asserts that the stack is one element high
 * and the only element that lives on it is
 * done and valid
 */
static inline err_t
parser_get (query *dest, parser *sp, error *e)
{
  parser_assert (sp);
  ASSERT (sp->sp == 1);

  ast_parser top = parser_pop (sp);
  ASSERT (top.type == SBBT_QUERY);
  stackp_result res = ast_parser_build (&top, &sp->ctrl->types_alloc, e);

  switch (res)
    {
    case SPR_SYNTAX_ERROR:
    case SPR_NOMEM:
      {
        return error_causef (
            e, (err_t)res,
            "Parser failed to build last element");
      }
    case SPR_CONTINUE:
      {
        UNREACHABLE ();
      }
    default:
      {
        break;
      }
    }

  *dest = top.qb.ret;

  return SUCCESS;
}

void
parser_release (parser *t)
{
  parser_assert (t);
}

void
parser_create (
    parser *dest,
    cbuffer *tokens_input,
    cbuffer *queries_output,
    stmtctrl *ctrl)
{
  *dest = (parser){
    .tokens_input = tokens_input,
    .queries_output = queries_output,

    .sp = 0,

    .working_space = lalloc_create_from (dest->_working_space),

    .ctrl = ctrl,
  };

  parser_begin (dest);

  parser_assert (dest);
}

static inline err_t
parser_write_out (parser *p, error *e)
{
  query res;

  err_t_wrap (parser_get (&res, p, e), e);

  u32 written = cbuffer_write (&res, sizeof res, 1, p->queries_output);
  ASSERT (written == 1);

  return SUCCESS;
}

void
parser_execute (parser *p)
{
  parser_assert (p);

  switch (p->ctrl->state)
    {
    case STCTRL_ERROR:
      {
        // Discard elements
        cbuffer_discard_all (p->tokens_input);
        return;
      }
    case STCTRL_WRITING:
      {
        // Expect to be done
        ASSERT (cbuffer_len (p->tokens_input) == 0);
        return;
      }
    case STCTRL_EXECTUING:
      {
        break;
      }
    }

  while (true)
    {
      /**
       * Block on downstream
       */
      if (cbuffer_avail (p->queries_output) < sizeof (query))
        {
          return;
        }

      /**
       * Block on upstream
       */
      if (cbuffer_len (p->tokens_input) == 0)
        {
          return;
        }

      token tok;
      u32 read = cbuffer_read (&tok, sizeof tok, 1, p->tokens_input);

      // Block on upstream
      if (read == 0)
        {
          return;
        }

      i_log_trace ("Parser got token: %s\n", tt_tostr (tok.type));

      switch (parser_feed_token (p, tok, &p->ctrl->e))
        {
        case SPR_DONE:
          {
            if (parser_write_out (p, &p->ctrl->e))
              {
                panic ();
              }
            break;
          }
        case SPR_CONTINUE:
          {
            continue; // Do nothing
          }
        case SPR_NOMEM:
        case SPR_SYNTAX_ERROR:
          {
            p->sp = 0;
            parser_begin (p);

            p->ctrl->state = STCTRL_ERROR;
            return;
          }
        }
    }
}
