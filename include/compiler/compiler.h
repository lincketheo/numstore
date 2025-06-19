#pragma once

#include "ast/query/query_provider.h" // query_provider
#include "ds/cbuffer.h"               // cbuffer
#include "intf/types.h"               // u32

/**
 * A single compiler combines both scan and parse steps
 *
 * Pattern:
 *
 * compiler* c = compiler_create(qp, e);
 *
 * cbuffer* input = compiler_get_input(c);
 * cbuffer* output = compiler_get_output(c);
 *
 * cbuffer_write(input, "create a struct { ", N);
 *
 * compiler_result res;
 * while(cbuffer_read(&res, sizeof(res), 1, output) != 0) {
 *    if(!res.ok) {
 *      handle_error(res.error);
 *    }
 * }
 *
 *
 */
typedef struct compiler_s compiler;

compiler *compiler_create (query_provider *qp, error *e);
void compiler_free (compiler *c);
cbuffer *compiler_get_input (compiler *c);
cbuffer *compiler_get_output (compiler *c);
void compiler_execute (compiler *s);
