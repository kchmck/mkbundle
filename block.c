// See copyright notice in Copying.

#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "block.h"
#include "ext-block.h"
#include "parser.h"
#include "primary-block.h"
#include "util.h"

#ifdef MKBUNDLE_TEST
#include "greatest.h"
#endif

void block_init(block_t *b) {
    *b = (block_t) {
        .type = BLOCK_TYPE_INVALID,
    };
}

void block_destroy(block_t *b) {
    switch (b->type) {
    case BLOCK_TYPE_PRIMARY:
        primary_block_destroy(&b->primary);
    break;

    case BLOCK_TYPE_EXT:
    break;

    case BLOCK_TYPE_INVALID:
    break;
    }
}

static block_type_t parse_block_type(parser_t *p) {
    static const char *MAP[] = {
        [BLOCK_TYPE_PRIMARY] = "primary",
        [BLOCK_TYPE_EXT] = "extension",
    };

    uint32_t type = parser_parse_sym(p, MAP, ASIZE(MAP));

    if (type == SYM_INVALID)
        return BLOCK_TYPE_INVALID;

    return (block_type_t) type;
}

#ifdef MKBUNDLE_TEST
TEST test_parse_block_type(void) {
    parser_t parser;
    parser_init(&parser);

    static const char J[] = "\"primary\": {\"a\": 0}";
    ASSERT(parser_parse(&parser, J, sizeof(J) - 1));

    ASSERT(parse_block_type(&parser) == BLOCK_TYPE_PRIMARY);
    ASSERT(parser_advance(&parser));

    ASSERT(parse_block_type(&parser) == BLOCK_TYPE_INVALID);

    PASS();
}
#endif

bool block_unserialize(block_t *b, const char *buf, size_t len) {
    parser_t parser;
    parser_init(&parser);

    if (!parser_parse(&parser, buf, len))
        return false;

    b->type = parse_block_type(&parser);

    switch (b->type) {
    case BLOCK_TYPE_PRIMARY:
        primary_block_init(&b->primary);
        primary_block_unserialize(&b->primary, &parser);
    break;

    case BLOCK_TYPE_EXT:
        ext_block_init(&b->ext);
        ext_block_unserialize(&b->ext, &parser);
    break;

    case BLOCK_TYPE_INVALID:
        return false;
    break;
    }

    return true;
}

void block_write(const block_t *b, FILE *stream) {
    switch (b->type) {
    case BLOCK_TYPE_PRIMARY:
        primary_block_write(&b->primary, stream);
    break;

    case BLOCK_TYPE_EXT:
        ext_block_write(&b->ext, stream);
    break;

    case BLOCK_TYPE_INVALID:
    break;
    }
}

#ifdef MKBUNDLE_TEST
SUITE(block_suite) {
    RUN_TEST(test_parse_block_type);
}
#endif
