#include "compiler/parser.h"

#include "dev/assert.h"  // DEFINE_DBG_ASSERT_I
#include "dev/testing.h" // TEST

DEFINE_DBG_ASSERT_I (parser, parser, p)
{
  ASSERT (p);
  ASSERT (p->tokens_input);
  ASSERT (p->tokens_input->cap % sizeof (token) == 0);
  ASSERT (p->query_ptr_output->cap % sizeof (query *) == 0);
}

// The thing that you can pop off of the stack
typedef struct
{
  sb_build_type type;

  union
  {
    type t;
    query *q;
  };
} ast_result;

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
ast_parser_accept_token (
    ast_parser *b,
    token t,
    lalloc *alloc,
    error *e)
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
                    b->tb.p = t.prim;
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
                    b->cur = t.q;

                    return SPR_CONTINUE;
                  }
                case TT_DELETE:
                  {
                    b->qb.state = QP_DELETE;
                    b->qb.dp = dltp_create ();
                    b->cur = t.q;

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
ast_parser_build (ast_parser *b, ast_result *dest, error *e)
{
  dest->type = b->type;

  switch (b->type)
    {
    case SBBT_TYPE:
      {
        switch (b->tb.state)
          {
          case TB_STRUCT:
            {
              dest->t.type = T_STRUCT;
              return kvp_build_struct (
                  &dest->t.st,
                  &b->tb.kvp,
                  &b->cur->alloc, e);
            }
          case TB_UNION:
            {
              dest->t.type = T_UNION;
              return kvp_build_union (
                  &dest->t.un,
                  &b->tb.kvp,
                  &b->cur->alloc, e);
            }
          case TB_ENUM:
            {
              dest->t.type = T_ENUM;
              return enp_build (
                  &dest->t.en,
                  &b->tb.enp,
                  &b->cur->alloc, e);
            }
          case TB_SARRAY:
            {
              dest->t.type = T_SARRAY;
              return sap_build (
                  &dest->t.sa,
                  &b->tb.sap,
                  &b->cur->alloc, e);
            }
          case TB_PRIM:
            {
              dest->t.type = T_PRIM;
              dest->t.p = b->tb.p;
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
              ASSERT (b->cur->type == QT_CREATE);
              dest->q = b->cur;
              return crtp_build (dest->q->create, &b->qb.cp, e);
            }
          case QP_DELETE:
            {
              ASSERT (b->cur->type == QT_DELETE);
              dest->q = b->cur;
              return dltp_build (dest->q->delete, &b->qb.dp, e);
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

//////////////////////////////////////////// UTILS

static inline stackp_result
parser_pop (parser *sp, ast_result *dest, error *e)
{
  parser_assert (sp);
  ASSERT (sp->sp > 0);
  ast_parser top = sp->stack[--sp->sp];
  stackp_result ret = ast_parser_build (&top, dest, e);
  lalloc_reset_to_state (&sp->working_space, top.alloc_start);
  return ret;
}

static inline ast_parser *
parser_top_ref (parser *sp)
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
parser_push_type_parser (parser *p, query *cur, error *e)
{
  parser_assert (p);

  if (p->sp == 20)
    {
      return error_causef (
          e, ERR_NOMEM,
          "Parser: stack overflow");
    }

  ast_parser value = {
    .tb = (type_parser){
        .state = TB_UNKNOWN,
    },
    .type = SBBT_TYPE,
    .cur = cur,
    .alloc_start = lalloc_get_state (&p->working_space),
  };

  parser_push_expect (p, value);
  return SUCCESS;
}

static void
parser_push_query_parser (parser *sp)
{
  parser_assert (sp);
  ASSERT (sp->sp == 0);

  ast_parser top = {
    .qb = (query_parser){
        .state = QP_UNKNOWN,
    },
    .type = SBBT_QUERY,
    .cur = NULL,
    .alloc_start = lalloc_get_state (&sp->working_space),
  };
  parser_push_expect (sp, top);
}

//////////////////////////////////////////// Main Subroutines
static stackp_result
parser_feed_token (parser *tp, token t, error *e)
{
  parser_assert (tp);
  ASSERT (tp->sp > 0);

  /**
   * First, get the top ast parser from the
   * stack - and ask: "Hey parser, do you
   * want a token or a type next?"
   */
  ast_parser *top = parser_top_ref (tp);
  sb_feed_t exp = ast_parser_expect_next (top, t);

  /**
   * Top Parser wants a type, we push a new
   * type builder onto the top of the stack and
   * by the way, all type parsers need to accept
   * a token first
   */
  if (exp == SBFT_TYPE)
    {
      err_t ret = parser_push_type_parser (tp, top->cur, e);
      if (ret < 0)
        {
          return (stackp_result)ret;
        }

      // Get the new top
      top = parser_top_ref (tp);

      // It definitely wants a token this time
      ASSERT (ast_parser_expect_next (top, t) == SBFT_TOKEN);
    }

  // Give the top a token
  stackp_result ret = ast_parser_accept_token (
      top, t,
      &tp->working_space, e);

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
      ast_result top;
      ret = parser_pop (tp, &top, e);
      if (ret < 0)
        {
          return ret;
        }

      /**
       * Done returns either DONE or ERROR, not CONTINUE
       */
      ASSERT (ret == SPR_DONE);

      /**
       * Merge whatever we just built
       * into the next element
       */
      switch (top.type)
        {
        case SBBT_TYPE:
          {
            ret = ast_parser_accept_type (
                &tp->stack[tp->sp - 1], top.t, e);
            break;
          }
        case SBBT_QUERY:
          {
            // Don't support recursive queries yet
            panic ();
            break;
          }
        }

      if (ret < 0)
        {
          return ret;
        }
      continue;
    }

  return ret;
}

/**
 * Asserts that the stack is one element high
 * and the only element that lives on it is
 * done and valid
 */
static inline err_t
parser_get (query **dest, parser *sp, error *e)
{
  parser_assert (sp);
  ASSERT (sp->sp == 1);

  /**
   * Pop the last element off the stack -
   * it should be a query
   */
  ast_result top;
  stackp_result res = parser_pop (sp, &top, e);
  ASSERT (top.type == SBBT_QUERY);

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
        UNREACHABLE (); // The last query should be done
      }
    default:
      {
        break;
      }
    }

  *dest = top.q;

  return SUCCESS;
}

static inline err_t
parser_write_out (parser *p, error *e)
{
  query *res;

  err_t_wrap (parser_get (&res, p, e), e);
  parser_push_query_parser (p);

  u32 written = cbuffer_write (&res, sizeof res, 1, p->query_ptr_output);
  ASSERT (written == 1);

  return SUCCESS;
}

static err_t
parser_feed_and_translate_token (parser *p, token tok, error *e)
{
  i_log_trace ("Parser got token: %s\n", tt_tostr (tok.type));

  switch (parser_feed_token (p, tok, e))
    {
    case SPR_DONE:
      {
        err_t_wrap (parser_write_out (p, e), e);
        return SUCCESS;
      }
    case SPR_CONTINUE:
      {
        return SUCCESS; // Do nothing
      }
    case SPR_NOMEM:
    case SPR_SYNTAX_ERROR:
      {
        return err_t_from (e);
      }
    }
  UNREACHABLE ();
}

//////////////////////////////////////////// Main Methods
void
parser_create (
    parser *dest,
    cbuffer *tokens_input,
    cbuffer *query_ptr_output)
{
  *dest = (parser){
    .tokens_input = tokens_input,
    .query_ptr_output = query_ptr_output,

    .sp = 0,

    .working_space = lalloc_create_from (dest->_working_space),

    .cur = NULL,
  };

  parser_push_query_parser (dest);

  parser_assert (dest);
}

err_t
parser_execute (parser *p, error *e)
{
  parser_assert (p);

  while (true)
    {
      /**
       * Block on downstream
       */
      if (cbuffer_avail (p->query_ptr_output) < sizeof (query *))
        {
          return SUCCESS;
        }

      /**
       * Block on upstream
       */
      if (cbuffer_len (p->tokens_input) == 0)
        {
          return SUCCESS;
        }

      token tok;
      u32 read = cbuffer_read (&tok, sizeof tok, 1, p->tokens_input);
      ASSERT (read > 0);

      err_t_wrap (parser_feed_and_translate_token (p, tok, e), e);
    }
}
