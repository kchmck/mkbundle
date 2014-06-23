// See copyright notice in Copying.

#ifndef PARSER_H
#define PARSER_H

#include <inttypes.h>
#include <stdbool.h>
#include <stdlib.h>

#include "eid.h"
#include "jsmn.h"

typedef struct {
    jsmntok_t tokens[256];
    const jsmntok_t *cur;
    size_t token;
    size_t token_count;
    const char *src;
    // Whether an error occured in parser functions that return a parsed value.
    bool error;
} parser_t;

// Initialize the given parser to a default state.
void parser_init(parser_t *p);

// Parse the given JSON. The parser takes ownership of the given buffer.
bool parser_parse(parser_t *p, const char *src, size_t len);

// Check if there are more tokens to visit.
static inline bool parser_more(const parser_t *p) {
    return p->token < p->token_count;
}

// Visit the next token. Return false if there are no more tokens left and true
// otherwise.
bool parser_advance(parser_t *p);

// Get a pointer into the source JSON referenced by the current token.
const char *parser_cur_str(const parser_t *p);

// Get the length of the string referenced by the current token.
size_t parser_cur_len(const parser_t *p);

// Parse the current token as a uint32_t. Abort on parse error.
uint32_t parser_parse_u32(parser_t *p);

// Parse the current token as a uint8_t. Abort on parse error.
uint8_t parser_parse_u8(parser_t *p);

// Parse the current token as one of the symbols in the given syms array. Return
// SYM_INVALID on parse error.
uint32_t parser_parse_sym(parser_t *p, const char **syms, size_t sym_count);

// Parse the current token as an EID. Abort on parse error.
bool parser_parse_eid(parser_t *p, eid_t *e);

#endif
