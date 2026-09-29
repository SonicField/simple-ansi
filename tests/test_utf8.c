#include "strip_ansi.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

static size_t encode_utf8(uint32_t code_point, unsigned char output[4]) {
    if (code_point <= 0x7ff) {
        output[0] = (unsigned char)(0xc0u | (code_point >> 6));
        output[1] = (unsigned char)(0x80u | (code_point & 0x3fu));
        return 2;
    }
    if (code_point <= 0xffff) {
        output[0] = (unsigned char)(0xe0u | (code_point >> 12));
        output[1] = (unsigned char)(0x80u | ((code_point >> 6) & 0x3fu));
        output[2] = (unsigned char)(0x80u | (code_point & 0x3fu));
        return 3;
    }
    output[0] = (unsigned char)(0xf0u | (code_point >> 18));
    output[1] = (unsigned char)(0x80u | ((code_point >> 12) & 0x3fu));
    output[2] = (unsigned char)(0x80u | ((code_point >> 6) & 0x3fu));
    output[3] = (unsigned char)(0x80u | (code_point & 0x3fu));
    return 4;
}

int main(void) {
    size_t checked = 0;

    for (uint32_t code_point = 0x80; code_point <= 0x10ffff; code_point++) {
        unsigned char encoded[4];
        unsigned char filtered[4];
        size_t encoded_len;
        size_t filtered_len = 0;
        ansi_filter filter;

        if (code_point >= 0xd800 && code_point <= 0xdfff) {
            continue;
        }

        encoded_len = encode_utf8(code_point, encoded);
        ansi_filter_init(&filter);
        for (size_t i = 0; i < encoded_len; i++) {
            size_t produced = 0;
            if (ansi_filter_process(&filter, encoded + i, 1,
                                    filtered + filtered_len,
                                    sizeof(filtered) - filtered_len,
                                    &produced) != ANSI_FILTER_OK) {
                fprintf(stderr, "filter error at U+%04X byte %zu\n",
                        code_point, i);
                return 1;
            }
            filtered_len += produced;
        }
        if (ansi_filter_finish(&filter) != ANSI_FILTER_OK ||
            filtered_len != encoded_len ||
            memcmp(filtered, encoded, encoded_len) != 0) {
            fprintf(stderr, "UTF-8 preservation failed at U+%04X\n",
                    code_point);
            return 1;
        }
        checked++;
    }

    printf("%zu valid non-ASCII Unicode scalars passed byte-by-byte\n", checked);
    return 0;
}
