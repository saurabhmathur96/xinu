#include <xinu.h>
#include <string.h>


void string_pair_table_init(string_pair_table_t* table, int size)
{
    table->size = size;
    table->n_entries = 0;
    table->entries = xmalloc(sizeof(*(table->entries)) * table->size);
    
    int i;
    for (i=0; i<table->size; i++)
    {
        table->entries[i] = (string_pair_t) { .key=NULL, .value=NULL };
    }
    
}

int string_pair_table_is_full(string_pair_table_t* table)
{
    return table->size == table->n_entries-1;
}

char *duplicate_string(char* string)
{
    int length = strlen(string);
    char* duplicate = xmalloc(sizeof(*duplicate) * (length+1));
    strncpy(duplicate, string, length);
}


int find_string_pair(string_pair_table_t *table, char *key)
{
    int i;
    for(i=0; i<table->size; i++) 
    {
        if(key == NULL)
        {
            if (table->entries[i].key == NULL )
            {
                // We were looking for a null key and we found a null key
                return i;
            }

        }
        else if (strcmp(table->entries[i].key, key) == 0) 
        {
            // We were looking for a string key and we found a match
            return i;
        }
    }

    // Not Found
    return -1;
}


int insert_back_string_pair(string_pair_table_t* table, char* key, char* value)
{
    if (string_pair_table_is_full(table))
    {
        return 1;
    }

    // Find first null entry
    int i = find_string_pair(table, NULL);
    if (i < 0) 
    {
        return 1;
    }

    table->entries[i] = (string_pair_t){ .key=key, .value=value };
    table->n_entries += 1;
    
    return 0;
}

int insert_front_string_pair(string_pair_table_t* table, char* key, char* value)
{
    if (string_pair_table_is_full(table))
    {
        return 1;
    }

    int i;
    for(i=table->size-1; i>=0; i--)
    {
        table->entries[i+1] = table->entries[i];
    }

    table->entries[0] = (string_pair_t){ .key=key, .value=value };
    table->n_entries += 1;

    
    return 0;
}

string_pair_t remove_string_pair(string_pair_table_t* table, int index)
{
    int target = index;

    string_pair_t removed_pair = table->entries[target];
    table->entries[target] = (string_pair_t) { .key=NULL, .value=NULL };

    int i;
    for(i=target; i<table->size-1; i++)
    {
        table->entries[i] = table->entries[i+1];
    }
    return removed_pair;
}


int kv_init()
{
    string_pair_table_init(&kv_store, KV_STORE_SIZE);
    return 0;
}

int kv_set(char* key, char* value)
{
    if (string_pair_table_is_full(&kv_store)) 
    {
        string_pair_t pair = remove_string_pair(&kv_store,  0);
        xfree(pair.key);
        xfree(pair.value);
    }
    return insert_back_string_pair(&kv_store, duplicate_string(key), duplicate_string(value));
    
}

char* kv_get(char* key)
{
    int target = find_string_pair(&kv_store, key);
    if (target<0)
    {
        return NULL;
    }
    string_pair_t pair = remove_string_pair(&kv_store, target);
    insert_front_string_pair(&kv_store, pair.key, pair.value);
    return pair.value;
}

int kv_delete(char* key)
{
    int index = find_string_pair(&kv_store, key);
    if (index < 0)
    {
        return 0;
    }
    string_pair_t pair = remove_string_pair(&kv_store, index);
    xfree(pair.key);
    xfree(pair.value);
    return 1;
}