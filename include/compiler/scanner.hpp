#pragma once

#include "compiler/tokens.hpp"

result<token_arr> scan(const char* data, usize dlen);
