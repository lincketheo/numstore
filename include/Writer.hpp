#pragma once

#include "Buffer.hpp"
#include "types.hpp"

// [(a, b, c), d, e, (f, g)]
// (non tuples are represented as len 1 tuples)
// e.g. the above is [(a, b, c), (d), (e), (f, g)] in memory
typedef struct
{
  Buffer *data_buffer;
  Buffer *index_buffer;
  usize var_size;
} write_var;

typedef struct
{
  write_var var;
  usize len;
} write_var_arr;

/**
 * A writer is a context aware writer of byte data given
 * the supplied arguments
 */
class Writer
{
public:
  Writer (Buffer *b);

  // Returns next bytes to write
  result<bytes> write_more (bytes b);

private:
  write_var_arr *vars;
  usize arrlen; // The number of tuples of variables
  usize len;    // The number of unique datapoints to store of each
};
