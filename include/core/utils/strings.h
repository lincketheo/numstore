#pragma once

#include "core/ds/strings.h"
#include "core/errors/error.h"
#include "core/intf/types.h" // TODO
#include "core/mm/lalloc.h"

u64 line_length (const char *buf, u64 max);
int strings_all_unique (const string *strs, u32 count);
bool string_equal (const string s1, const string s2);
string string_plus (const string left, const string right, lalloc *alloc, error *e);
const string *strings_are_disjoint (const string *left, u32 llen, const string *right, u32 rlen);

// Lexical comparison
bool string_less_string (const string left, const string right);
bool string_greater_string (const string left, const string right);
bool string_less_equal_string (const string left, const string right);
bool string_greater_equal_string (const string left, const string right);
