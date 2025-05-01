#include "compiler/tokens.h"

void
i_log_token (token t)
{
  string tokt = tt_tostr (t.type);
  switch (t.type)
    {
    case TT_IDENTIFIER:
      {
        i_log_info ("%.*s %.*s\n",
                    tokt.len, tokt.data, t.str.len, t.str.data);
        break;
      }
    case TT_STRING:
      {
        i_log_info ("%.*s %.*s\n",
                    tokt.len, tokt.data, t.str.len, t.str.data);
        break;
      }
    case TT_FLOAT:
      {
        i_log_info ("%.*s %f\n", tokt.len, tokt.data, t.floating);
        break;
      }
    case TT_INTEGER:
      {
        i_log_info ("%.*s %d\n", tokt.len, tokt.data, t.integer);
        break;
      }
    case TT_PRIM:
      {
        i_log_info ("%.*s %s\n", tokt.len, tokt.data, prim_to_str (t.prim));
        break;
      }
    default:
      {
        i_log_info ("%.*s\n", tokt.len, tokt.data);
        break;
      }
    }
}

string
tt_tostr (token_t t)
{
  switch (t)
    {
      // Tokens that start with a letter (alpha)
      //      Operations
    case TT_CREATE:
      return (string){ .data = "TT_CREATE", .len = 9 };
    case TT_DELETE:
      return (string){ .data = "TT_DELETE", .len = 9 };
    case TT_APPEND:
      return (string){ .data = "TT_APPEND", .len = 9 };
    case TT_INSERT:
      return (string){ .data = "TT_INSERT", .len = 9 };
    case TT_UPDATE:
      return (string){ .data = "TT_UPDATE", .len = 9 };
    case TT_READ:
      return (string){ .data = "TT_READ", .len = 7 };
    case TT_TAKE:
      return (string){ .data = "TT_TAKE", .len = 7 };

      //      Binary Operations
    case TT_BCREATE:
      return (string){ .data = "TT_BCREATE", .len = 9 };
    case TT_BDELETE:
      return (string){ .data = "TT_BDELETE", .len = 9 };
    case TT_BAPPEND:
      return (string){ .data = "TT_BAPPEND", .len = 9 };
    case TT_BINSERT:
      return (string){ .data = "TT_BINSERT", .len = 9 };
    case TT_BUPDATE:
      return (string){ .data = "TT_BUPDATE", .len = 9 };
    case TT_BREAD:
      return (string){ .data = "TT_BREAD", .len = 7 };
    case TT_BTAKE:
      return (string){ .data = "TT_BTAKE", .len = 7 };

      //      Type Utils
    case TT_STRUCT:
      return (string){ .data = "TT_STRUCT", .len = 9 };
    case TT_UNION:
      return (string){ .data = "TT_UNION", .len = 8 };
    case TT_ENUM:
      return (string){ .data = "TT_ENUM", .len = 7 };
    case TT_PRIM:
      return (string){ .data = "TT_PRIM", .len = 7 };

      //      Other
    case TT_IDENTIFIER:
      return (string){ .data = "TT_IDENTIFIER", .len = 13 };
    case TT_STRING:
      return (string){ .data = "TT_STRING", .len = 13 };

      // Tokens that start with a number or +/-
    case TT_INTEGER:
      return (string){ .data = "TT_INTEGER", .len = 9 };
    case TT_FLOAT:
      return (string){ .data = "TT_FLOAT", .len = 8 };

      // Tokens that are single characters
    case TT_SEMICOLON:
      return (string){ .data = "TT_SEMICOLON", .len = 12 };
    case TT_LEFT_BRACKET:
      return (string){ .data = "TT_LEFT_BRACKET", .len = 15 };
    case TT_RIGHT_BRACKET:
      return (string){ .data = "TT_RIGHT_BRACKET", .len = 16 };
    case TT_LEFT_BRACE:
      return (string){ .data = "TT_LEFT_BRACE", .len = 13 };
    case TT_RIGHT_BRACE:
      return (string){ .data = "TT_RIGHT_BRACE", .len = 14 };
    case TT_LEFT_PAREN:
      return (string){ .data = "TT_LEFT_PAREN", .len = 13 };
    case TT_RIGHT_PAREN:
      return (string){ .data = "TT_RIGHT_PAREN", .len = 14 };
    case TT_COMMA:
      return (string){ .data = "TT_COMMA", .len = 8 };
    }
  ASSERT (0);
  return (string){ .data = NULL, .len = 0 };
}
