#include "ansi_io.h"

#include <errno.h>

#include "strip_ansi.h"

enum { IO_BUFFER_SIZE = 16384 };

static int write_all(const ansi_io *io, const unsigned char *buffer,
                     size_t length) {
    size_t written = 0;

    while (written < length) {
        ssize_t result = io->write(io->write_context, buffer + written,
                                   length - written);
        if (result < 0) {
            if (errno == EINTR) {
                continue;
            }
            return ANSI_IO_WRITE_ERROR;
        }
        if (result == 0 || (size_t)result > length - written) {
            errno = EIO;
            return ANSI_IO_WRITE_ERROR;
        }
        written += (size_t)result;
    }
    return ANSI_IO_OK;
}

int ansi_filter_copy(const ansi_io *io) {
    unsigned char input[IO_BUFFER_SIZE];
    unsigned char output[IO_BUFFER_SIZE];
    ansi_filter filter;

    if (io == NULL || io->read == NULL || io->write == NULL) {
        errno = EINVAL;
        return ANSI_IO_INVALID_ARGUMENT;
    }

    ansi_filter_init(&filter);
    for (;;) {
        ssize_t read_result = io->read(io->read_context, input, sizeof(input));
        size_t output_len = 0;
        int filter_result;
        int write_result;

        if (read_result < 0) {
            if (errno == EINTR) {
                continue;
            }
            return ANSI_IO_READ_ERROR;
        }
        if (read_result == 0) {
            filter_result = ansi_filter_finish(&filter);
            if (filter_result != ANSI_FILTER_OK) {
                errno = EINVAL;
                return ANSI_IO_FILTER_ERROR;
            }
            return ANSI_IO_OK;
        }

        filter_result = ansi_filter_process(&filter, input,
                                            (size_t)read_result,
                                            output, sizeof(output),
                                            &output_len);
        if (filter_result != ANSI_FILTER_OK) {
            errno = EINVAL;
            return ANSI_IO_FILTER_ERROR;
        }
        write_result = write_all(io, output, output_len);
        if (write_result != ANSI_IO_OK) {
            return write_result;
        }
    }
}
