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
    pair *table[TABLE_SIZE];
} hash_table;

unsigned int hash(char *key);
hash_table create_table();
void table_insert(hash_table *table, char *key, void *value, size_t size);
pair *table_get(hash_table *table, char *key);
void free_table(hash_table *table);