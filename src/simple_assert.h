/*
 * Assertions are executable specifications and remain enabled in all builds.
 */
#ifndef SIMPLE_ASSERT_H
#define SIMPLE_ASSERT_H

#include <stdio.h>
#include <stdlib.h>

#define ASSERT_MSG(condition, format, ...) do { \
    if (!(condition)) { \
        fprintf(stderr, "ASSERT FAILED %s:%d: " format "\n", \
                __FILE__, __LINE__, ##__VA_ARGS__); \
        abort(); \
    } \
} while (0)

#endif
