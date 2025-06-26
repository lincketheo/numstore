#include "compiler/tokens.h"

#include "core/dev/assert.h" // UNREACHABLE

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

const char *
tt_tostr (token_t t)
{
  switch (t)
    {
      // Tokens that start with a letter (alpha)
      //      Operations
    case TT_CREATE:
      return "TT_CREATE";
    case TT_DELETE:
      return "TT_DELETE";
    case TT_APPEND:
      return "TT_APPEND";
    case TT_INSERT:
      return "TT_INSERT";
    case TT_UPDATE:
      return "TT_UPDATE";
    case TT_READ:
      return "TT_READ";
    case TT_TAKE:
      return "TT_TAKE";

      //      Binary Operations
    case TT_BCREATE:
      return "TT_BCREATE";
    case TT_BDELETE:
      return "TT_BDELETE";
    case TT_BAPPEND:
      return "TT_BAPPEND";
    case TT_BINSERT:
      return "TT_BINSERT";
    case TT_BUPDATE:
      return "TT_BUPDATE";
    case TT_BREAD:
      return "TT_BREAD";
    case TT_BTAKE:
      return "TT_BTAKE";

      //      Type Utils
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

      //      Other
    case TT_IDENTIFIER:
      return "TT_IDENTIFIER";
    case TT_STRING:
      return "TT_STRING";

      // Tokens that start with a number or +/-
    case TT_INTEGER:
      return "TT_INTEGER";
    case TT_FLOAT:
      return "TT_FLOAT";

      // Tokens that are single characters
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
    }
  UNREACHABLE ();
  return NULL;
}
