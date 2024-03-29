#pragma once
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#define TABLE_SIZE 100

typedef struct
{
    char *key;
    void *value;
    struct pair *next;
} pair;

typedef struct
{
    int size;
    pair *table[TABLE_SIZE];
} hash_table;

unsigned int hash(char *key);
hash_table create_table();
hash_table table_from_array(char **values, size_t size);
void table_insert(hash_table *table, char *key, void *value, size_t size);
pair *table_get(hash_table *table, char *key);
void free_table(hash_table *table);