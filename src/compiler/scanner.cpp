#include "compiler/scanner.hpp"
#include "macros.hpp"
#include "testing.hpp"

#include <cstdlib>

#define INITIAL_CAP 100

class Scanner
{
public:
  Scanner (const char *data, usize len);

  result<void> scan (token_arr *arr);

private:
#define scanner_assert()                                                      \
  assert (data);                                                              \
  assert (dlen > 0);                                                          \
  assert (start <= current)

  bool is_end ();
  char peek_char ();
  char next_char ();
  void skip_whitespace ();
  void scan_over_integer ();
  result<token_t> check_keyword ();
  token_t parse_alpha ();
  result<token_t> scan_next_token_t ();
  result<token> scan_next_token ();

  const char *data;
  usize dlen;

  usize start;
  usize current;
  bool error;
};

result<token_arr>
scan (const char *data, usize dlen)
{
  result<token_arr> arr = token_arr_create ();
  if (!arr.ok)
    {
      return err<token_arr> ();
    }

  Scanner s (data, dlen);
  if (!s.scan (&arr.value).ok)
    {
      return err<token_arr> ();
    }

  return ok (arr.value);
}

Scanner::Scanner (const char *_data, usize _dlen) : data (_data), dlen (_dlen)
{
  start = 0;
  current = 0;
  error = false;
  scanner_assert ();
}

result<void>
Scanner::scan (token_arr *arr)
{
  scanner_assert ();
  token_arr_assert (arr);

  while (true)
    {
      // Scan next token
      result<token> t = scan_next_token ();
      if (!t.ok)
        {
          return err<void> ();
        }

      // Push next token
      result<void> res = token_arr_push (arr, t.value);
      if (!res.ok)
        {
          return err<void> ();
        }

      if (t.value.type == T_EOF)
        {
          break;
        }
    }

  if (error)
    {
      return err<void> ();
    }

  return ok_void ();
}

bool
Scanner::is_end ()
{
  scanner_assert ();

  return current == dlen;
}

char
Scanner::peek_char ()
{
  scanner_assert ();

  if (is_end ())
    {
      return EOF;
    }
  return data[current];
}

char
Scanner::next_char ()
{
  scanner_assert ();
  assert (!is_end ());

  return data[current++];
}

void
Scanner::skip_whitespace ()
{
  scanner_assert ();

  while (true)
    {
      if (is_end ())
        {
          return;
        }

      // Check next whitespace
      char c = peek_char ();
      switch (c)
        {
        case WHITESPACE_NL_C:
          {
            next_char (); // ignore return - we know it's just whitespace
            break;
          }
        default:
          {
            return;
          }
        }
    }
}

void
Scanner::scan_over_integer ()
{
  scanner_assert ();

  while (is_digit (peek_char ()))
    {
      next_char (); // ignore return - captured in final atoi
    }
}

static struct
{
  const char *str;
  token_t t;
  usize slen;
} keywords[] = {
  { "write", T_WRITE, 5 },
  { nullptr, T_EOF },
};

result<token_t>
Scanner::check_keyword ()
{
  scanner_assert ();
  assert (current > start);

  usize dlen = current - start;
  for (int i = 0; keywords[i].str; ++i)
    {
      if (keywords[i].slen != dlen)
        {
          continue;
        }

      if (strncmp (data, keywords[i].str, keywords[i].slen) == 0)
        {
          return ok (keywords[i].t);
        }
    }

  return err<token_t> ();
}

token_t
Scanner::parse_alpha ()
{
  scanner_assert ();

  // Scan over all alpha numeric chars
  while (is_alpha_num (peek_char ()))
    {
      next_char (); // ignore output - part of the string
    }

  // Check if it's a keyword
  result<token_t> kw = check_keyword ();
  if (kw.ok)
    {
      return kw.value;
    }

  return T_IDENT;
}

result<token>
Scanner::scan_next_token ()
{
  scanner_assert ();

  result<token_t> t = scan_next_token_t ();
  if (!t.ok)
    {
      return err<token> ();
    }

  return ok (token_create (&data[start], current - start, t.value, start));
}

result<token_t>
Scanner::scan_next_token_t ()
{
  scanner_assert ();

  skip_whitespace ();
  start = current;

  if (is_end ())
    {
      return ok (T_EOF);
    }

  char c = next_char ();
  switch (c)
    {
    case '(':
      {
        return ok (T_LEFT_PAREN);
      }
    case ')':
      {
        return ok (T_RIGHT_PAREN);
      }
    case '[':
      {
        return ok (T_LEFT_BRACKET);
      }
    case ']':
      {
        return ok (T_RIGHT_BRACKET);
      }
    case ',':
      {
        return ok (T_COMMA);
      }
    default:
      {
        if (is_digit (c))
          {
            scan_over_integer ();
            return ok (T_INTEGER);
          }
        else if (is_alpha (c))
          {
            return ok (parse_alpha ());
          }
        else
          {
            return err<token_t> ();
          }
      }
    }
}

TEST (scan_tests)
{
  const char *s1 = "WRITE 5 [a, (,,,(,bc,WRITE5,5WRITE,9]]),[)";
  result<token_arr> t = scan (s1, strlen (s1));

  test_assert_equal (t.ok, true, "%d");

  if (t.ok)
    {
      token_arr_print (&t.value);
    }
}
