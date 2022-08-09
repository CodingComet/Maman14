#include "hash_table.h"

/* Fowler-Noll-Vo hash function */
unsigned int hash(char *key)
{
    int length = strlen(key);
    unsigned int hash = 0x811c9dc5;

    int i = 0;
    for (; i < length; i++)
        hash = ((hash ^ key[i]) * 0x01000193);

    return hash % TABLE_SIZE;
}

hash_table create_table()
{
    hash_table table;
    table.size = 0;

    int i = 0;
    for (; i < TABLE_SIZE; i++)
        table.table[i] = NULL;

    return table;
}

hash_table table_from_array(char **values, size_t size)
{
    hash_table table = create_table();

    int i = 0;
    for (; i < TABLE_SIZE; i++)
        table.table[i] = NULL;
    for (i = 0; i < size; i++)
        table_insert(&table, values[i], &i, sizeof(int));

    return table;
}

void table_insert(hash_table *table, char *key, void *value, size_t size)
{
    int index = hash(key);

    pair *new_pair = (pair *)malloc(sizeof(pair));

    /* Copying the input */
    new_pair->key = malloc(strlen(key) + 1);
    memcpy(new_pair->key, key, strlen(key) + 1);

    new_pair->value = malloc(size);
    memcpy(new_pair->value, value, size);


    if (!table->table[index])
    {
        table->table[index] = new_pair;
        new_pair->next = NULL;
    }
    else
    {
        new_pair->next = table->table[index];
        table->table[index] = new_pair;
    }

    table->size++;
}

pair *table_get(hash_table *table, char *key)
{
    if (!key)
        return NULL;
    int index = hash(key);
    pair *p = table->table[index];

    while (p)
    {
        if (0 == strcmp(p->key, key))
            return p;
        p = p->next;
    }
    return NULL;
}

void free_table(hash_table *table)
{
    int i = 0;
    for (; i < TABLE_SIZE; i++)
    {
        pair *p = table->table[i];
        while (p)
        {
            pair *tmp = p;
            p = p->next;
            free(tmp->key);
            free(tmp->value);
            free(tmp);
        }
    }
}
