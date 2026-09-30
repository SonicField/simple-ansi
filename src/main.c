#include "ansi_io.h"

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define SIMPLE_ANSI_VERSION "0.1.0"

static int usage(FILE *stream) {
    return fprintf(stream, "Usage: simple-ansi [--help] [--version] [FILE]\n");
}

static int finish_standard_output(void) {
    if (fflush(stdout) == 0) {
        return 0;
    }
    fprintf(stderr, "simple-ansi: write failed: %s\n", strerror(errno));
    return 1;
}

static ssize_t read_fd(void *context, unsigned char *buffer, size_t capacity) {
    const int fd = *(const int *)context;
    return read(fd, buffer, capacity);
}

static ssize_t write_fd(void *context, const unsigned char *buffer,
                        size_t length) {
    const int fd = *(const int *)context;
    return write(fd, buffer, length);
}

int main(int argc, char **argv) {
    const char *path = NULL;
    int input_fd = STDIN_FILENO;
    int output_fd = STDOUT_FILENO;
    int must_close = 0;
    ansi_io io;
    int result;
    int operation_errno;

    if (signal(SIGPIPE, SIG_IGN) == SIG_ERR) {
        fprintf(stderr, "simple-ansi: cannot configure output handling: %s\n",
                strerror(errno));
        return 1;
    }

    if (argc == 2 && strcmp(argv[1], "--help") == 0) {
        if (usage(stdout) < 0) {
            fprintf(stderr, "simple-ansi: write failed: %s\n", strerror(errno));
            return 1;
        }
        return finish_standard_output();
    }
    if (argc == 2 && strcmp(argv[1], "--version") == 0) {
        if (printf("simple-ansi %s\n", SIMPLE_ANSI_VERSION) < 0) {
            fprintf(stderr, "simple-ansi: write failed: %s\n", strerror(errno));
            return 1;
        }
        return finish_standard_output();
    }
    if (argc == 2 && strcmp(argv[1], "--") == 0) {
        /* Explicit end of options with no filename still means stdin. */
    } else if (argc == 2 && argv[1][0] != '-') {
        path = argv[1];
    } else if (argc == 3 && strcmp(argv[1], "--") == 0) {
        path = argv[2];
    } else if (argc != 1) {
        fprintf(stderr, "simple-ansi: invalid arguments\n");
        usage(stderr);
        return 2;
    }

    if (path != NULL) {
        input_fd = open(path, O_RDONLY);
        if (input_fd < 0) {
            fprintf(stderr, "simple-ansi: cannot open '%s': %s\n",
                    path, strerror(errno));
            return 1;
        }
        must_close = 1;
    }

    io.read = read_fd;
    io.read_context = &input_fd;
    io.write = write_fd;
    io.write_context = &output_fd;
    result = ansi_filter_copy(&io);
    operation_errno = errno;

    if (must_close && close(input_fd) != 0 && result == ANSI_IO_OK) {
        fprintf(stderr, "simple-ansi: cannot close '%s': %s\n",
                path, strerror(errno));
        return 1;
    }

    if (result == ANSI_IO_READ_ERROR) {
        if (path == NULL) {
            fprintf(stderr, "simple-ansi: read failed: %s\n",
                    strerror(operation_errno));
        } else {
            fprintf(stderr, "simple-ansi: read failed for '%s': %s\n",
                    path, strerror(operation_errno));
        }
        return 1;
    }
    if (result == ANSI_IO_WRITE_ERROR) {
        fprintf(stderr, "simple-ansi: write failed: %s\n",
                strerror(operation_errno));
        return 1;
    }
    if (result != ANSI_IO_OK) {
        fprintf(stderr, "simple-ansi: internal filter failure\n");
        return 1;
    }
    return 0;
}
