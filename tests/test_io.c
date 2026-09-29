#include "ansi_io.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    const unsigned char *data;
    size_t length;
    size_t at;
    size_t max_transfer;
    int interrupt_once;
    int fail;
} input_fixture;

typedef struct {
    unsigned char data[256];
    size_t length;
    size_t max_transfer;
    int interrupt_once;
    int fail;
    int return_zero;
} output_fixture;

static ssize_t fixture_read(void *context, unsigned char *buffer, size_t capacity) {
    input_fixture *fixture = context;
    size_t amount;

    if (fixture->interrupt_once) {
        fixture->interrupt_once = 0;
        errno = EINTR;
        return -1;
    }
    if (fixture->fail) {
        errno = EIO;
        return -1;
    }
    if (fixture->at == fixture->length) {
        return 0;
    }
    amount = fixture->length - fixture->at;
    if (amount > capacity) {
        amount = capacity;
    }
    if (amount > fixture->max_transfer) {
        amount = fixture->max_transfer;
    }
    memcpy(buffer, fixture->data + fixture->at, amount);
    fixture->at += amount;
    return (ssize_t)amount;
}

static ssize_t fixture_write(void *context, const unsigned char *buffer,
                             size_t length) {
    output_fixture *fixture = context;
    size_t amount = length;

    if (fixture->interrupt_once) {
        fixture->interrupt_once = 0;
        errno = EINTR;
        return -1;
    }
    if (fixture->fail) {
        errno = EIO;
        return -1;
    }
    if (fixture->return_zero) {
        return 0;
    }
    if (amount > fixture->max_transfer) {
        amount = fixture->max_transfer;
    }
    if (amount > sizeof(fixture->data) - fixture->length) {
        errno = ENOSPC;
        return -1;
    }
    memcpy(fixture->data + fixture->length, buffer, amount);
    fixture->length += amount;
    return (ssize_t)amount;
}

static int check(const char *label, int condition) {
    if (!condition) {
        fprintf(stderr, "FAIL: %s\n", label);
        return 0;
    }
    printf("PASS: %s\n", label);
    return 1;
}

int main(void) {
    static const unsigned char input[] =
        "before\x1b[31mred\x1b[0m/\x1b]title\x07" "after";
    static const unsigned char expected[] = "beforered/after";
    int passed = 1;

    {
        input_fixture source = {
            input, sizeof(input) - 1, 0, 3, 1, 0
        };
        output_fixture sink = {{0}, 0, 2, 1, 0, 0};
        ansi_io io = {fixture_read, &source, fixture_write, &sink};

        errno = 0;
        passed &= check("short and interrupted I/O completes",
                        ansi_filter_copy(&io) == ANSI_IO_OK);
        passed &= check("short writes preserve exact output",
                        sink.length == sizeof(expected) - 1 &&
                        memcmp(sink.data, expected, sizeof(expected) - 1) == 0);
    }

    {
        input_fixture source = {input, sizeof(input) - 1, 0, 8, 0, 1};
        output_fixture sink = {{0}, 0, 8, 0, 0, 0};
        ansi_io io = {fixture_read, &source, fixture_write, &sink};

        passed &= check("permanent read error is reported",
                        ansi_filter_copy(&io) == ANSI_IO_READ_ERROR);
    }

    {
        input_fixture source = {input, sizeof(input) - 1, 0, 8, 0, 0};
        output_fixture sink = {{0}, 0, 8, 0, 1, 0};
        ansi_io io = {fixture_read, &source, fixture_write, &sink};

        passed &= check("permanent write error is reported",
                        ansi_filter_copy(&io) == ANSI_IO_WRITE_ERROR);
    }

    {
        input_fixture source = {input, sizeof(input) - 1, 0, 8, 0, 0};
        output_fixture sink = {{0}, 0, 8, 0, 0, 1};
        ansi_io io = {fixture_read, &source, fixture_write, &sink};

        passed &= check("zero-length write is an error",
                        ansi_filter_copy(&io) == ANSI_IO_WRITE_ERROR);
    }

    {
        ansi_io missing_read = {NULL, NULL, fixture_write, NULL};
        ansi_io missing_write = {fixture_read, NULL, NULL, NULL};

        passed &= check("NULL I/O descriptor rejected",
                        ansi_filter_copy(NULL) == ANSI_IO_INVALID_ARGUMENT);
        passed &= check("NULL read function rejected",
                        ansi_filter_copy(&missing_read) == ANSI_IO_INVALID_ARGUMENT);
        passed &= check("NULL write function rejected",
                        ansi_filter_copy(&missing_write) == ANSI_IO_INVALID_ARGUMENT);
    }

    return passed ? 0 : 1;
}
