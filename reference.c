// Reference implementation of vector addition without OpenCL
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

const size_t VECTOR_DIMENSIONS = 100000;

int main(void) {
    _Bool ok = 0;

    printf("Allocating memory for vectors...\n");
    volatile float *vec_a, *vec_b, *vec_result;
    vec_a = vec_b = vec_result = NULL;
    vec_a = malloc(sizeof(float) * VECTOR_DIMENSIONS);
    vec_b = malloc(sizeof(float) * VECTOR_DIMENSIONS);
    vec_result = malloc(sizeof(float) * VECTOR_DIMENSIONS);
    if (vec_a == NULL || vec_b == NULL || vec_result == NULL)
        goto cleanup_alloc;

    srand(time(NULL));
    for (size_t i = 0; i < VECTOR_DIMENSIONS; i++) {
        vec_a[i] = rand() / (float)RAND_MAX;
        vec_b[i] = rand() / (float)RAND_MAX;
    }

    printf("Summing vectors...\n");
    for (size_t i = 0; i < VECTOR_DIMENSIONS; i++) {
        vec_result[i] = vec_a[i] + vec_b[i];
    }

    ok = 1;

    cleanup_alloc:
        free((void *)vec_a);
        free((void *)vec_b);
        free((void *)vec_result);
    if (ok) {
        printf("Done.\n");
        return EXIT_SUCCESS;
    }
    printf("Failed.\n");
    return EXIT_FAILURE;
}