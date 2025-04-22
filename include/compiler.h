#pragma once

#include "dev/assert.h"
#include "intf/mm.h"
#include "sds.h"
#include "typing.h"

/**
 * Compiler Architecture
 * Data In (chars) -> Scanner -> Tokens -> Parser -> ByteCode -> VM
 */

////////////////////// TOKENS
/**
 * Tokens are little logical blocks of information
 * that are 1 step higher than "chars". Some tokens
 * require more than just 1 char (strings, floats etc),
 * this step of the compilation process converts a string
 * of raw chars into their object forms (e.g. a stream of
 * numbers into an integer, or a stream of specific chars
 * to magic words - write, read etc - or identifiers.
 */
typedef enum
{
  // Tokens that start with a letter (alpha)
  TT_WRITE,
  TT_CREATE,
  TT_IDENTIFIER,

  // Tokens that start with a number or +/-
  TT_INTEGER,
  TT_FLOAT,

  // Types
  TT_U32,

  // Tokens that are single characters
  TT_SEMICOLON,

  // Special Tokens
  TT_ERROR, // An Error token, saying: next token start fresh
} token_t;

// Returns if this token is a single character
#define TT_IS_SINGLE(t) (t == TT_SEMICOLON)

/**
 * A token is a tagged union that wraps
 * the value and the type together
 */
typedef struct
{
  union
  {
    string str;
    i32 integer;
    f32 floating;
  };
  u8 type;
} token;

#define quick_tok(_type) \
  (token) { .type = _type }
#define tt_integer(val) \
  (token) { .type = TT_INTEGER, .integer = val }
#define tt_float(val) \
  (token) { .type = TT_FLOAT, .floating = val }
#define tt_ident(val) \
  (token) { .type = TT_IDENTIFIER, .str = val }

////////////////////// SCANNER (chars -> tokens)

/**
 * The scanner has an input circular buffer of chars,
 * and outputs tokens to the output token buffer.
 *
 * - current is the distance from the tail of the buffer
 *   to the currently scanned token. You'll see other scanners
 *   have start and current, this one treats "start" as chars_input[0]
 *   (as a circular buffer) and current is the leading index. This means
 *   you need to be careful about when you "consume" buffer values. In
 *   general, buffer values are only consumed when the scanner is done parsing
 *   one token, then current gets reset to 0
 * - is_error - TODO - After scanning an error (which is actually pretty rare considering
 *   the scanner doesn't deal with syntax, biggest error will be unknown starting
 *   character). Figure out what to do here
 * - state/strings that exceed input buffer capacity - TODO
 */
typedef enum
{
  SS_START,
  SS_CHAR_COLLECT,
  SS_NUMBER_COLLECT,
  SS_DECIMAL_COLLECT,
  SS_ERROR_REWIND,
} scanner_state;

typedef struct
{
  cbuffer *chars_input;
  cbuffer *tokens_output;
  u32 current;
  scanner_state state;

  char *dcur;    // Current data for variable length data
  u32 dcurlen;   // len of dcur
  u32 dcurcap;   // capacity of dcur
  lalloc *alloc; // Limited Allocator
} scanner;

DEFINE_DBG_ASSERT_H (scanner, scanner, s);
void scanner_create (
    scanner *dest,
    cbuffer *input,
    cbuffer *output, lalloc *alloc);
void scanner_execute (scanner *s); // Buffer will be empty at the end

////////////////////// PARSER

typedef struct
{
  cbuffer *tokens_input;
  cbuffer *bytecode_output;
  int is_error;
} parser;

DEFINE_DBG_ASSERT_H (parser, parser, p);
void parser_create (parser *dest, cbuffer *input, cbuffer *output);
void parser_execute (parser *s);
