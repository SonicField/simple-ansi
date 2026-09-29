#ifndef STRIP_ANSI_H
#define STRIP_ANSI_H

#include <stddef.h>

/*
 * Public streaming state. Initialise with ansi_filter_init(), pass every input
 * chunk to ansi_filter_process(), then call ansi_filter_finish() exactly once.
 * Callers should not inspect or modify these fields.
 */
typedef enum {
    ANSI_FILTER_TEXT = 0,
    ANSI_FILTER_ESCAPE,
    ANSI_FILTER_ESCAPE_INTERMEDIATE,
    ANSI_FILTER_CSI,
    ANSI_FILTER_STRING,
    ANSI_FILTER_STRING_ESCAPE
} ansi_filter_mode;

typedef struct {
    ansi_filter_mode mode;
    unsigned char utf8_remaining;
    unsigned char utf8_first;
    unsigned char utf8_min;
    unsigned char utf8_max;
    unsigned char string_allows_bel;
    int finished;
} ansi_filter;

enum {
    ANSI_FILTER_OK = 0,
    ANSI_FILTER_INVALID_ARGUMENT = 1,
    ANSI_FILTER_FINISHED = 2
};

/* A NULL pointer is ignored, allowing defensive cleanup-style initialisation. */
void ansi_filter_init(ansi_filter *filter);

/*
 * Filter one chunk.
 *
 * output_capacity must be at least input_len; the filter never expands input.
 * input and output may point to the same buffer. NULL buffers are accepted only
 * for an empty chunk. On success, output_len receives the bytes produced.
 */
int ansi_filter_process(ansi_filter *filter,
                        const unsigned char *input,
                        size_t input_len,
                        unsigned char *output,
                        size_t output_capacity,
                        size_t *output_len);

/*
 * Discard any incomplete terminal sequence and close the stream. Processing or
 * finishing this state again returns ANSI_FILTER_FINISHED.
 */
int ansi_filter_finish(ansi_filter *filter);

/*
 * Compatibility helper for NUL-terminated strings. It filters in place,
 * appends a new NUL terminator, and returns the output length. Use the streaming
 * API for arbitrary bytes, including embedded NUL.
 */
size_t strip_ansi(char *text);

#endif
