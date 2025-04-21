#pragma once

#define is_alpha(c) (((c) >= 'a' && (c) <= 'z')    \
                     || ((c) >= 'A' && (c) <= 'Z') \
                     || (c) == '_')
#define is_num(c) ((c) >= '0' && (c) <= '9')
#define is_alpha_num(c) (is_alpha (c) || is_num (c))
