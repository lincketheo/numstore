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
  TT_READ,
  TT_TAKE,
  TT_CREATE,
  TT_DELETE,
  TT_IDENTIFIER,

  // Tokens that start with a number or +/-
  TT_INTEGER,
  TT_FLOAT,

  // Types
  //      Complex
  TT_STRUCT,
  TT_UNION,
  TT_ENUM,
  TT_PRIM,

  // Tokens that are single characters
  TT_SEMICOLON,
  TT_LEFT_BRACKET,
  TT_RIGHT_BRACKET,
  TT_LEFT_BRACE,
  TT_RIGHT_BRACE,
  TT_LEFT_PAREN,
  TT_RIGHT_PAREN,
  TT_COMMA,

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
    prim_t prim;
  };
  token_t type;
} token;

#define quick_tok(_type) \
  (token) { .type = _type }
#define tt_integer(val) \
  (token) { .type = TT_INTEGER, .integer = val }
#define tt_float(val) \
  (token) { .type = TT_FLOAT, .floating = val }
#define tt_ident(val) \
  (token) { .type = TT_IDENTIFIER, .str = val }
#define tt_prim(val) \
  (token) { .type = TT_PRIM, .prim = val }

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

  char *dcur;            // Current data for variable length data
  u32 dcurlen;           // len of dcur
  u32 dcurcap;           // capacity of dcur
  lalloc *strings_alloc; // Allocator for strings
} scanner;

DEFINE_DBG_ASSERT_H (scanner, scanner, s);
void scanner_create (
    scanner *dest,
    cbuffer *input,
    cbuffer *output,
    lalloc *strings_alloc);
void scanner_execute (scanner *s); // Buffer will be empty at the end

////////////////////// PARSER

typedef enum
{
  PS_START,
  PS_CREATE,
  PS_ERROR_REWIND,
} parser_state;

typedef struct
{
  cbuffer *tokens_input;  // The input buffer for tokens
  cbuffer *opcode_output; // The output for op codes
  cbuffer *stack_output;  // The stack output for the vm

  lalloc *input_strings; // The string allocator
  lalloc *tokens;        // The type allocator

  type *currenttype;  // While building types
  parser_state state; // Current state for execute
} parser;

DEFINE_DBG_ASSERT_H (parser, parser, p);
void parser_create (
    parser *dest,
    cbuffer *input,
    cbuffer *output,
    lalloc *input_strings,
    lalloc *tokens);

void parser_execute (parser *s);

////////////////////// TYPE PARSER

/**
 * T -> p |
 *      struct { i T K } |
 *      union { i T K } |
 *      enum { i I } |
 *      A T
 *
 * A -> V | S
 * K -> \eps | , T K
 * V -> [] V | []
 * S -> [NUM] S | [NUM]
 * I -> \eps | , i I
 *
 * Note:
 * T = TYPE
 * A = ARRAY_PREFIX
 * K = KEY_VALUE_LIST
 * V = VARRAY_BRACKETS
 * S = SARRAY_BRACKETS
 * I = ENUM_KEY_LIST
 */

typedef enum
{
  TPR_EXPECT_NEXT_TOKEN,
  TPR_EXPECT_NEXT_TYPE,
  TPR_SYNTAX_ERROR,
  TPR_DONE,
} tp_result;

typedef struct
{
  enum
  {
    SB_WAITING_FOR_LB,
    SB_WAITING_FOR_IDENT,
    SB_WAITING_FOR_TYPE,
    SB_WAITING_FOR_COMMA_OR_RIGHT,
  } state;

} struct_builder;

typedef struct
{

  enum
  {
    UB_WAITING_FOR_LB,
    UB_WAITING_FOR_IDENT,
    UB_WAITING_FOR_TYPE,
    UB_WAITING_FOR_COMMA_OR_RIGHT,
  } state;

} union_builder;

typedef struct
{

  enum
  {
    TB_UNSURE,
    TB_STRUCT,
    TB_UNION,
  } type;

  union
  {
    struct_builder sb;
    union_builder ub;
  };

  type *ret;

} type_builder;

typedef struct
{
  type_builder *stack;
  u32 sp;
} type_parser;

DEFINE_DBG_ASSERT_H (type_parser, type_parser, tp);

err_t tp_create (type_parser *dest, lalloc *token_allocator);

tp_result tp_feed_token (type_parser *tp, token t);
