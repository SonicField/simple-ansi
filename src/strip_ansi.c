/* Streaming terminal-control byte filter. */

#include "strip_ansi.h"

#include <string.h>

#include "simple_assert.h"

static int is_cancel(unsigned char byte) {
    return byte == 0x18 || byte == 0x1a;
}

static int is_csi_final(unsigned char byte) {
    return byte >= 0x40 && byte <= 0x7e;
}

static void begin_string(ansi_filter *filter, int allows_bel) {
    filter->mode = ANSI_FILTER_STRING;
    filter->string_allows_bel = (unsigned char)(allows_bel != 0);
}

static void begin_utf8(ansi_filter *filter, unsigned char byte) {
    if (byte <= 0xdf) {
        filter->utf8_remaining = 1;
        filter->utf8_min = 0x80;
        filter->utf8_max = 0xbf;
    } else if (byte == 0xe0) {
        filter->utf8_remaining = 2;
        filter->utf8_min = 0xa0;
        filter->utf8_max = 0xbf;
    } else if (byte <= 0xec) {
        filter->utf8_remaining = 2;
        filter->utf8_min = 0x80;
        filter->utf8_max = 0xbf;
    } else if (byte == 0xed) {
        filter->utf8_remaining = 2;
        filter->utf8_min = 0x80;
        filter->utf8_max = 0x9f;
    } else if (byte <= 0xef) {
        filter->utf8_remaining = 2;
        filter->utf8_min = 0x80;
        filter->utf8_max = 0xbf;
    } else if (byte == 0xf0) {
        filter->utf8_remaining = 3;
        filter->utf8_min = 0x90;
        filter->utf8_max = 0xbf;
    } else if (byte <= 0xf3) {
        filter->utf8_remaining = 3;
        filter->utf8_min = 0x80;
        filter->utf8_max = 0xbf;
    } else {
        filter->utf8_remaining = 3;
        filter->utf8_min = 0x80;
        filter->utf8_max = 0x8f;
    }
    filter->utf8_first = 1;
}

void ansi_filter_init(ansi_filter *filter) {
    if (filter == NULL) {
        return;
    }
    memset(filter, 0, sizeof(*filter));
    filter->mode = ANSI_FILTER_TEXT;
}

int ansi_filter_process(ansi_filter *filter,
                        const unsigned char *input,
                        size_t input_len,
                        unsigned char *output,
                        size_t output_capacity,
                        size_t *output_len) {
    size_t input_at = 0;
    size_t output_at = 0;

    if (filter == NULL || output_len == NULL ||
        (input_len != 0 && (input == NULL || output == NULL)) ||
        output_capacity < input_len) {
        return ANSI_FILTER_INVALID_ARGUMENT;
    }
    *output_len = 0;
    if (filter->finished) {
        return ANSI_FILTER_FINISHED;
    }

    while (input_at < input_len) {
        unsigned char byte = input[input_at];

        if (filter->mode == ANSI_FILTER_TEXT && filter->utf8_remaining != 0) {
            unsigned char minimum = filter->utf8_first ? filter->utf8_min : 0x80;
            unsigned char maximum = filter->utf8_first ? filter->utf8_max : 0xbf;
            if (byte >= minimum && byte <= maximum) {
                output[output_at++] = byte;
                input_at++;
                filter->utf8_remaining--;
                filter->utf8_first = 0;
                continue;
            }
            filter->utf8_remaining = 0;
            filter->utf8_first = 0;
            continue;
        }

        switch (filter->mode) {
        case ANSI_FILTER_TEXT:
            if (byte == 0x1b) {
                filter->mode = ANSI_FILTER_ESCAPE;
            } else if (byte == 0x9b) {
                filter->mode = ANSI_FILTER_CSI;
            } else if (byte == 0x9d) {
                begin_string(filter, 1);
            } else if (byte == 0x90 || byte == 0x98 ||
                       byte == 0x9e || byte == 0x9f) {
                begin_string(filter, 0);
            } else if (byte == '\t' || byte == '\n') {
                output[output_at++] = byte;
            } else if (byte < 0x20 || (byte >= 0x7f && byte <= 0x9f)) {
                /* Strip C0 (except HT/LF), DEL, and standalone C1. */
            } else {
                output[output_at++] = byte;
                if (byte >= 0xc2 && byte <= 0xf4) {
                    begin_utf8(filter, byte);
                }
            }
            input_at++;
            break;

        case ANSI_FILTER_ESCAPE:
            if (byte == '[') {
                filter->mode = ANSI_FILTER_CSI;
                input_at++;
            } else if (byte == ']') {
                begin_string(filter, 1);
                input_at++;
            } else if (byte == 'P' || byte == 'X' ||
                       byte == '^' || byte == '_') {
                begin_string(filter, 0);
                input_at++;
            } else if (byte >= 0x20 && byte <= 0x2f) {
                filter->mode = ANSI_FILTER_ESCAPE_INTERMEDIATE;
                input_at++;
            } else if (byte >= 0x30 && byte <= 0x7e) {
                filter->mode = ANSI_FILTER_TEXT;
                input_at++;
            } else if (byte == 0x1b) {
                input_at++;
            } else if (is_cancel(byte)) {
                filter->mode = ANSI_FILTER_TEXT;
                input_at++;
            } else {
                filter->mode = ANSI_FILTER_TEXT;
            }
            break;

        case ANSI_FILTER_ESCAPE_INTERMEDIATE:
            if (byte >= 0x20 && byte <= 0x2f) {
                input_at++;
            } else if (byte >= 0x30 && byte <= 0x7e) {
                filter->mode = ANSI_FILTER_TEXT;
                input_at++;
            } else if (byte == 0x1b) {
                filter->mode = ANSI_FILTER_ESCAPE;
                input_at++;
            } else if (is_cancel(byte)) {
                filter->mode = ANSI_FILTER_TEXT;
                input_at++;
            } else {
                filter->mode = ANSI_FILTER_TEXT;
            }
            break;

        case ANSI_FILTER_CSI:
            if (is_csi_final(byte) || is_cancel(byte)) {
                filter->mode = ANSI_FILTER_TEXT;
            }
            input_at++;
            break;

        case ANSI_FILTER_STRING:
            if ((byte == 0x07 && filter->string_allows_bel) ||
                byte == 0x9c || is_cancel(byte)) {
                filter->mode = ANSI_FILTER_TEXT;
            } else if (byte == 0x1b) {
                filter->mode = ANSI_FILTER_STRING_ESCAPE;
            }
            input_at++;
            break;

        case ANSI_FILTER_STRING_ESCAPE:
            if (byte == '\\' ||
                (byte == 0x07 && filter->string_allows_bel) ||
                byte == 0x9c || is_cancel(byte)) {
                filter->mode = ANSI_FILTER_TEXT;
            } else if (byte != 0x1b) {
                filter->mode = ANSI_FILTER_STRING;
            }
            input_at++;
            break;
        }
    }

    ASSERT_MSG(output_at <= input_len,
               "ansi_filter_process: output length %zu exceeds input length %zu; "
               "a stripping filter must not invent bytes",
               output_at, input_len);
    *output_len = output_at;
    return ANSI_FILTER_OK;
}

int ansi_filter_finish(ansi_filter *filter) {
    if (filter == NULL) {
        return ANSI_FILTER_INVALID_ARGUMENT;
    }
    if (filter->finished) {
        return ANSI_FILTER_FINISHED;
    }
    filter->mode = ANSI_FILTER_TEXT;
    filter->utf8_remaining = 0;
    filter->utf8_first = 0;
    filter->finished = 1;
    return ANSI_FILTER_OK;
}

size_t strip_ansi(char *text) {
    ansi_filter filter;
    size_t original_len;
    size_t output_len = 0;
    int result;

    ASSERT_MSG(text != NULL,
               "strip_ansi: text must be non-NULL; caller passed NULL. "
               "Check the caller's buffer allocation or argument passing.");
    original_len = strlen(text);
    ansi_filter_init(&filter);
    result = ansi_filter_process(&filter, (const unsigned char *)text,
                                 original_len, (unsigned char *)text,
                                 original_len, &output_len);
    ASSERT_MSG(result == ANSI_FILTER_OK,
               "strip_ansi: streaming filter returned %d for valid in-place input; "
               "the compatibility wrapper violated its API preconditions",
               result);
    result = ansi_filter_finish(&filter);
    ASSERT_MSG(result == ANSI_FILTER_OK,
               "strip_ansi: finishing filter returned %d after one process call; "
               "the filter lifecycle invariant was violated",
               result);
    text[output_len] = '\0';
    ASSERT_MSG(output_len <= original_len,
               "strip_ansi: output length %zu exceeds input length %zu; "
               "stripping must never grow the string",
               output_len, original_len);
    return output_len;
}
