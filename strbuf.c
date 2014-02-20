// See copyright notice in Copying.

#include <assert.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>

#include "strbuf.h"

static strbuf_t *alloc(strbuf_t *sb, size_t cap) {
    sb = realloc(sb, sizeof(strbuf_t) + cap);
    assert(sb);

    sb->cap = cap;

    return sb;
}

void strbuf_init(strbuf_t **sbp, size_t cap) {
    strbuf_t *sb;

    sb = alloc(NULL, cap);
    sb->pos = 0;
    sb->buf[0] = 0;

    *sbp = sb;
}

void strbuf_init_buf(strbuf_t **sbp, const uint8_t *buf, size_t len) {
    strbuf_t *sb;

    sb = alloc(NULL, len + 1);
    sb->pos = len;
    memcpy(sb->buf, buf, len);
    sb->buf[len] = 0;

    *sbp = sb;
}

void strbuf_destroy(strbuf_t *sb) {
    free(sb);
}

void strbuf_destroy_buf(uint8_t *buf) {
    if (buf)
        free(buf - sizeof(strbuf_t));
}

void strbuf_expect(strbuf_t **sbp, size_t len) {
    strbuf_t *sb = *sbp;

    if (sb->pos + len > sb->cap)
        *sbp = alloc(sb, sb->cap + len);
}

void strbuf_append(strbuf_t **sbp, const uint8_t *buf, size_t len) {
    strbuf_t *sb;

    strbuf_expect(sbp, len);
    sb = *sbp;

    memcpy(&sb->buf[sb->pos], buf, len);
    sb->pos += len;
}

static void strbuf_append_char(strbuf_t **sbp, uint8_t c) {
    strbuf_t *sb;

    strbuf_expect(sbp, 1);
    sb = *sbp;

    sb->buf[sb->pos] = c;
    sb->pos += 1;
}

void strbuf_finish(strbuf_t **sbp) {
    strbuf_append_char(sbp, 0);
}
