#ifndef VECTOR_H
#define VECTOR_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#ifndef VECTOR_DATA_TYPE
#define VECTOR_DATA_TYPE int
#endif

typedef struct
{
    VECTOR_DATA_TYPE *data;
    size_t size;
    size_t capacity;
} vector;

vector create_vector(size_t initial_capacity);
bool realloc_vector(vector *v, size_t new_capacity);
bool vector_push_back(vector *v, VECTOR_DATA_TYPE value);
void free_vector(vector *v);

#endif
