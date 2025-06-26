#include "compiler/tokens.h"

#include "core/dev/assert.h" // UNREACHABLE

#include "core/ds/strings.h"
#include "core/errors/error.h"
#include "core/intf/logging.h"
#include "numstore/query/query.h" // QT_...

query_t
tt_to_qt (token_t t)
{
  switch (t)
    {
    case TT_CREATE:
      {
        return QT_CREATE;
      }
    case TT_DELETE:
      {
        return QT_DELETE;
      }
    case TT_INSERT:
      {
        return QT_INSERT;
      }
    default:
      {
        UNREACHABLE ();
      }
    }
}

void
i_log_token (token t)
{
  const char *tokt = tt_tostr (t.type);
  switch (t.type)
    {
    case TT_IDENTIFIER:
      {
        i_log_info ("%s %.*s\n", tokt, t.str.len, t.str.data);
        break;
      }
    case TT_STRING:
      {
        i_log_info ("%s %.*s\n", tokt, t.str.len, t.str.data);
        break;
      }
    case TT_FLOAT:
      {
        i_log_info ("%s %f\n", tokt, t.floating);
        break;
      }
    case TT_INTEGER:
      {
        i_log_info ("%s %d\n", tokt, t.integer);
        break;
      }
    case TT_PRIM:
      {
        i_log_info ("%s %s\n", tokt, prim_to_str (t.prim));
        break;
      }
    default:
      {
        i_log_info ("%s\n", tokt);
        break;
      }
    }
}

bool
token_equal (const token *left, const token *right)
{
  if (left->type != right->type)
    {
      return false;
    }

  switch (left->type)
    {
    //      Other
    case TT_STRING:
    case TT_IDENTIFIER:
      return string_equal (left->str, right->str);

    // Tokens that start with a number or +/-
    case TT_INTEGER:
      return left->integer == right->integer;
    case TT_FLOAT:
      return left->floating == right->floating;

    //      Literal Operations
    case TT_CREATE:
    case TT_DELETE:
    case TT_INSERT:
      return query_equal (&left->q, &right->q);

    case TT_ERROR:
      return error_equal (&left->e, &right->e);

    default:
      return true;
    }
}

const char *
tt_tostr (token_t t)
{
  switch (t)
    {
    //      Arithmetic Operators
    case TT_PLUS:
      return "TT_PLUS";
    case TT_MINUS:
      return "TT_MINUS";
    case TT_SLASH:
      return "TT_SLASH";
    case TT_STAR:
      return "TT_STAR";

    //      Logical Operators
    case TT_BANG:
      return "TT_BANG";
    case TT_BANG_EQUAL:
      return "TT_BANG_EQUAL";
    case TT_EQUAL_EQUAL:
      return "TT_EQUAL_EQUAL";
    case TT_GREATER:
      return "TT_GREATER";
    case TT_GREATER_EQUAL:
      return "TT_GREATER_EQUAL";
    case TT_LESS:
      return "TT_LESS";
    case TT_LESS_EQUAL:
      return "TT_LESS_EQUAL";

    //      Fancy Operators
    case TT_NOT:
      return "TT_NOT";
    case TT_CARET:
      return "TT_CARET";
    case TT_PERCENT:
      return "TT_PERCENT";
    case TT_PIPE:
      return "TT_PIPE";
    case TT_AMPERSAND:
      return "TT_AMPERSAND";

    //      Other One char tokens
    case TT_SEMICOLON:
      return "TT_SEMICOLON";
    case TT_COLON:
      return "TT_COLON";
    case TT_LEFT_BRACKET:
      return "TT_LEFT_BRACKET";
    case TT_RIGHT_BRACKET:
      return "TT_RIGHT_BRACKET";
    case TT_LEFT_BRACE:
      return "TT_LEFT_BRACE";
    case TT_RIGHT_BRACE:
      return "TT_RIGHT_BRACE";
    case TT_LEFT_PAREN:
      return "TT_LEFT_PAREN";
    case TT_RIGHT_PAREN:
      return "TT_RIGHT_PAREN";
    case TT_COMMA:
      return "TT_COMMA";

    //      Other
    case TT_STRING:
      return "TT_STRING";
    case TT_IDENTIFIER:
      return "TT_IDENTIFIER";

    // Tokens that start with a number or +/-
    case TT_INTEGER:
      return "TT_INTEGER";
    case TT_FLOAT:
      return "TT_FLOAT";

    //      Literal Operations
    case TT_CREATE:
      return "TT_CREATE";
    case TT_DELETE:
      return "TT_DELETE";
    case TT_INSERT:
      return "TT_INSERT";

    //      Type literals
    case TT_STRUCT:
      return "TT_STRUCT";
    case TT_UNION:
      return "TT_UNION";
    case TT_ENUM:
      return "TT_ENUM";
    case TT_PRIM:
      return "TT_PRIM";

    //      Bools
    case TT_TRUE:
      return "TT_TRUE";
    case TT_FALSE:
      return "TT_FALSE";

    case TT_ERROR:
      return "TT_ERROR";
    }

  UNREACHABLE ();
  return NULL;
}
