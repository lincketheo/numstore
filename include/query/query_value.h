#pragma once

#include "ds/strings.h"
#include "value/values/enum.h"

/**
 * {
 *   "foo" : [1, 2, 3],
 *   "bar" : [{"a" : 1}, {"b" : 2 }, {"c" : 3}]
 *   "biz" : ["FOO", "BAR", "BIZ"]
 *   "baz" : [1.1, 2.3, 5.6]
 * }
 */
typedef struct
{
  string *vnames; // "foo" "bar" "biz"
  u32 nvars;      // 4
  u32 nvals;      // 3
  value *values;  // len = nvars * nvals
} query_value;
