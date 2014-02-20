// See copyright notice in Copying.

#ifndef STRBUF_H
#define STRBUF_H

#include <stdlib.h>
#include <inttypes.h>

// A string buffer that can grow dynamically.
typedef struct {
    // The maximum capacity of the buffer.
    size_t cap;
    // The current position in the buffer.
    size_t pos;
    // The buffer itself.
    uint8_t buf[];
} strbuf_t;

// Initialize the given strbuf to have the given capacity.
void strbuf_init(strbuf_t **sbp, size_t cap);

// Initialize the given strbuf with a copy of the given character buffer with
// the given length.
void strbuf_init_buf(strbuf_t **sbp, const uint8_t *buf, size_t len);

// Free the memory held by the given strbuf.
void strbuf_destroy(strbuf_t *sb);

// Destroy the strbuf that wraps the given character buffer.
void strbuf_destroy_buf(uint8_t *buf);

// Ensure the given strbuf can hold len more bytes.
void strbuf_expect(strbuf_t **sbp, size_t len);

// Append the given string to the given strbuf. Note that no null-termination is
// done.
void strbuf_append(strbuf_t **sbp, const uint8_t *buf, size_t len);

// Add a null-terminator to the current position in the given strbuf.
void strbuf_finish(strbuf_t **sbp);

#endif
