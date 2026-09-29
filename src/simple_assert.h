/*
 * Assertions are executable specifications and remain enabled in all builds.
 */
#ifndef SIMPLE_ASSERT_H
#define SIMPLE_ASSERT_H

#include <stdio.h>
#include <stdlib.h>

#define ASSERT_MSG(condition, ...) do { \
    if (!(condition)) { \
        fprintf(stderr, "ASSERT FAILED %s:%d: ", __FILE__, __LINE__); \
        fprintf(stderr, __VA_ARGS__); \
        fputc('\n', stderr); \
        abort(); \
    } \
} while (0)

#endif
