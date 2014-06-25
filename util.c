// See copyright notice in Copying.

#include <inttypes.h>
#include <string.h>

#include "util.h"

#ifdef MKBUNDLE_TEST
#include "greatest.h"
#endif

uint32_t sym_parse(const char *str, const sym_t *syms, size_t sym_count) {
    for (size_t i = 0; i < sym_count; i += 1)
        if (strcmp(syms[i].sym, str) == 0)
            return syms[i].val;

    return SYM_INVALID;
}

#ifdef MKBUNDLE_TEST
TEST test_sym_parse(void) {
    enum { SYM1, SYM2 };
    static const sym_t MAP[] = {
        {SYM1, "sym1"},
        {SYM2, "sym2"},
        {SYM1, "sym3"},
    };

    ASSERT_EQ(sym_parse("sym1", MAP, ASIZE(MAP)), SYM1);
    ASSERT_EQ(sym_parse("sym2", MAP, ASIZE(MAP)), SYM2);
    ASSERT_EQ(sym_parse("sym", MAP, ASIZE(MAP)), SYM_INVALID);
    ASSERT_EQ(sym_parse("sym3", MAP, ASIZE(MAP)), SYM1);

    PASS();
}
#endif

void collect(strbuf_t **buf, FILE *stream) {
    char next[BUFSIZ];

    while (!feof(stream)) {
        size_t size = fread(next, sizeof(char), ASIZE(next), stream);
        strbuf_append(buf, next, size);
    }
}

#ifdef MKBUNDLE_TEST
SUITE(util_suite) {
    RUN_TEST(test_sym_parse);
}
#endif
