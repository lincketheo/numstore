#pragma once

#include "database.h"
#include "dev/assert.h"
#include "intf/mm.h"
#include "sds.h"
#include "typing.h"

////////////////////// SCANNER (chars -> tokens)

typedef struct scanner_s scanner;

err_t scnr_create (scanner **dest, database *db, cbuffer *inchars);
void scnr_execute (scanner *s);
void scnr_release (scanner *dest, database *db);

cbuffer *scnr_get_output_tokens (scanner *s);
lalloc *scnr_get_string_alloc (scanner *s);

////////////////////// PARSER (tokens -> opcodes)

typedef struct parser_s parser;

void parser_create (database *db, cbuffer *intoks, lalloc *salloc);
void parser_execute (parser *p);
void parser_release (parser *p);
