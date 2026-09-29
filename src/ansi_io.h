#ifndef ANSI_IO_H
#define ANSI_IO_H

#include <stddef.h>
#include <sys/types.h>

typedef ssize_t (*ansi_read_function)(void *context, unsigned char *buffer,
                                      size_t capacity);
typedef ssize_t (*ansi_write_function)(void *context,
                                       const unsigned char *buffer,
                                       size_t length);

typedef struct {
    ansi_read_function read;
    void *read_context;
    ansi_write_function write;
    void *write_context;
} ansi_io;

enum {
    ANSI_IO_OK = 0,
    ANSI_IO_READ_ERROR = 1,
    ANSI_IO_WRITE_ERROR = 2,
    ANSI_IO_FILTER_ERROR = 3,
    ANSI_IO_INVALID_ARGUMENT = 4
};

int ansi_filter_copy(const ansi_io *io);

#endif
