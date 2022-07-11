#include "vector.h"

vector create_vector(size_t initial_capacity)
{
    vector v;

    v.data = malloc(sizeof(VECTOR_DATA_TYPE) * initial_capacity);
    v.size = 0;
    v.capacity = initial_capacity;

    return v;
}

bool realloc_vector(vector *v, size_t new_capacity)
{
    VECTOR_DATA_TYPE *realloced_data = (VECTOR_DATA_TYPE *)realloc(v->data, new_capacity * sizeof(VECTOR_DATA_TYPE));
    if (!realloced_data)
        return false;

    v->data = realloced_data;
    v->capacity = new_capacity;

    return true;
}

bool vector_push_back(vector *v, VECTOR_DATA_TYPE value)
{
    if (v->size == v->capacity)
    {
        int ok = realloc_vector(v, v->capacity * 2);
        if (!ok)
            return false;
    }

    v->data[v->size++] = value;
    return true;
}

void free_vector(vector *v)
{
    free(v->data);
    v->size = 0;
    v->capacity = 0;
}
