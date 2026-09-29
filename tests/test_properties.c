#include "strip_ansi.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static uint32_t next_random(uint32_t *state) {
    uint32_t value = *state;
    value ^= value << 13;
    value ^= value >> 17;
    value ^= value << 5;
    *state = value;
    return value;
}

static int filter_partitioned(const unsigned char *input, size_t input_len,
                              unsigned char *output, size_t *output_len,
                              uint32_t seed, int one_chunk) {
    ansi_filter filter;
    size_t input_at = 0;
    size_t output_at = 0;

    ansi_filter_init(&filter);
    while (input_at < input_len) {
        size_t chunk = one_chunk ? input_len : (size_t)(next_random(&seed) % 31u) + 1;
        size_t produced = 0;
        if (chunk > input_len - input_at) {
            chunk = input_len - input_at;
        }
        if (ansi_filter_process(&filter, input + input_at, chunk,
                                output + output_at, input_len - output_at,
                                &produced) != ANSI_FILTER_OK) {
            return 0;
        }
        input_at += chunk;
        output_at += produced;
    }
    if (input_len == 0) {
        size_t produced = 1;
        if (ansi_filter_process(&filter, NULL, 0, NULL, 0,
                                &produced) != ANSI_FILTER_OK ||
            produced != 0) {
            return 0;
        }
    }
    if (ansi_filter_finish(&filter) != ANSI_FILTER_OK) {
        return 0;
    }
    *output_len = output_at;
    return 1;
}

static int is_subsequence(const unsigned char *input, size_t input_len,
                          const unsigned char *output, size_t output_len) {
    size_t output_at = 0;
    for (size_t input_at = 0; input_at < input_len && output_at < output_len;
         input_at++) {
        if (input[input_at] == output[output_at]) {
            output_at++;
        }
    }
    return output_at == output_len;
}

int main(void) {
    enum { CASES = 1000, MAX_LEN = 2048 };
    unsigned char *input = malloc(MAX_LEN);
    unsigned char *one_shot = malloc(MAX_LEN);
    unsigned char *partitioned = malloc(MAX_LEN);
    unsigned char *twice = malloc(MAX_LEN);
    uint32_t random_state = UINT32_C(0x5eed1234);

    if (input == NULL || one_shot == NULL || partitioned == NULL || twice == NULL) {
        fprintf(stderr, "allocation failed for property-test buffers\n");
        free(input);
        free(one_shot);
        free(partitioned);
        free(twice);
        return 1;
    }

    for (size_t test_case = 0; test_case < CASES; test_case++) {
        size_t input_len = (size_t)(next_random(&random_state) % MAX_LEN);
        size_t one_shot_len = 0;
        size_t partitioned_len = 0;
        size_t twice_len = 0;
        uint32_t partition_seed = next_random(&random_state);

        for (size_t i = 0; i < input_len; i++) {
            input[i] = (unsigned char)next_random(&random_state);
        }

        if (!filter_partitioned(input, input_len, one_shot, &one_shot_len,
                                UINT32_C(1), 1) ||
            !filter_partitioned(input, input_len, partitioned, &partitioned_len,
                                partition_seed, 0)) {
            fprintf(stderr, "filter failed in case %zu\n", test_case);
            return 1;
        }
        if (one_shot_len != partitioned_len ||
            memcmp(one_shot, partitioned, one_shot_len) != 0) {
            fprintf(stderr,
                    "chunk-independence failed in case %zu (seed %u)\n",
                    test_case, partition_seed);
            return 1;
        }
        if (one_shot_len > input_len ||
            !is_subsequence(input, input_len, one_shot, one_shot_len)) {
            fprintf(stderr, "non-expansion/subsequence failed in case %zu\n",
                    test_case);
            return 1;
        }
        if (!filter_partitioned(one_shot, one_shot_len, twice, &twice_len,
                                UINT32_C(2), 0) ||
            twice_len != one_shot_len ||
            memcmp(twice, one_shot, one_shot_len) != 0) {
            fprintf(stderr, "idempotence failed in case %zu\n", test_case);
            return 1;
        }
    }

    free(input);
    free(one_shot);
    free(partitioned);
    free(twice);
    printf("1000 deterministic adversarial streams passed\n");
    return 0;
}
