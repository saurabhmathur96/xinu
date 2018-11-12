#include <xinu.h>

/**
 * String Table API
 */

void print_string_table(string_pair_table_t* table)
{
    int i;
    for (i=0; i<table->size; i++)
    {
        printf("%d %s %s\n", i, table->entries[i].key, table->entries[i].value);
    }
}

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

void string_pair_table_destroy(string_pair_table_t* table)
{
    table->size = 0;
    table->n_entries = 0;
    
    int i;
    for (i=0; i<table->size; i++)
    {
        if(table->entries[i].key != NULL)
        {
            xfree(table->entries[i].key);
        }
        if(table->entries[i].value != NULL)
        {
            xfree(table->entries[i].value);
        }
    }

    xfree(table->entries);
}

int string_pair_table_is_full(string_pair_table_t* table)
{
    return table->size == table->n_entries;
}

char *duplicate_string(char* string)
{
    int length = strlen(string);
    char* duplicate = xmalloc(sizeof(*duplicate) * (length+1));
    strncpy(duplicate, string, length);
    return duplicate;
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
        else if (strncmp(table->entries[i].key, key, strlen(table->entries[i].key)) == 0) 
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
    for(i=target; i<table->n_entries; i++)
    {
        table->entries[i] = table->entries[i+1];
    }
    table->entries[i].key = NULL;

    table->n_entries -= 1;
    
    return removed_pair;
}


/**
 * LRU Cache implementation
 */

int lru_kv_set(string_pair_table_t* store, char* key, char* value)
{
    if (string_pair_table_is_full(lru_kv_store)) 
    {
        string_pair_t pair = remove_string_pair(store,  0);
        kv_stats.cache_size -= (strlen(pair.key) + strlen(pair.value));
        xfree(pair.key);
        xfree(pair.value);
        kv_stats.total_evictions++;
    }
    

    // check if key is already in cache
    int i = find_string_pair(store, key);
    if (i >= 0) 
    {
        // found => update value and set as most recently used
        string_pair_t pair = remove_string_pair(store,  i);
        kv_stats.cache_size += (strlen(value) - strlen(pair.value)) ;
        xfree(pair.value);

        return insert_back_string_pair(store, pair.key, duplicate_string(value));
    }

    // not found 
    kv_stats.cache_size += (strlen(key) + strlen(value));
    kv_stats.num_keys += 1;
    return insert_back_string_pair(store, duplicate_string(key), duplicate_string(value));
}

char* lru_kv_get(string_pair_table_t* store, char* key)
{
    int target = find_string_pair(store, key);
    if (target<0)
    {
        return NULL;
    }
    string_pair_t pair = remove_string_pair(store, target);
    insert_back_string_pair(store, pair.key, pair.value);
    return pair.value;
}

int lru_kv_delete(string_pair_table_t* store, char* key)
{
    int index = find_string_pair(store, key);
    if (index < 0)
    {
        return 0;
    }
    string_pair_t pair = remove_string_pair(store, index);
    kv_stats.cache_size -= (strlen(pair.key) + strlen(pair.value));
    kv_stats.num_keys -= 1;
    xfree(pair.key);
    xfree(pair.value);
    return 1;
}


int arc_kv_set(char* key, char* value)
{
    //
    return 0;
}

char* arc_kv_get(char* key)
{
    //
    return NULL
}


int arc_kv_delete(char* key)
{
    return 0;
}

/**
 * Key value pair API
 */

int kv_init(replacement_policy_t policy)
{
    xmalloc_init();
    kv_stats = (kv_stats_t) {
        .total_hits=0,
        .total_accesses=0,
        .total_set_success=0,
        .cache_size=0,
        .num_keys=0,
        .total_evictions=0
    };
    if (policy == LRU)
    {
        kv_replacement_policy = policy;
        string_pair_table_init(&lru_kv_store, KV_STORE_SIZE);
    }
    return 0;
}

char* kv_get(char* key)
{
    kv_stats.total_accesses++;
    
    char* return_value;
    switch(kv_replacement_policy)
    {
        case LRU:
            return_value = lru_kv_get(&lru_kv_store, key);

        default:
            return_value = lru_kv_get(&lru_kv_store, key);
    }
    if (return_value != NULL)
    {
        kv_stats.total_hits++;
    }
    return return_value;
}

int kv_set(char* key, char* value)
{
    int return_value;
    switch(kv_replacement_policy)
    {
        case LRU:
            return_value = lru_kv_set(&lru_kv_store, key, value);
            break;

        default:
            return_value = lru_kv_set(&lru_kv_store, key, value);
            break;
    }

    if (return_value == 0)
    {
        kv_stats.total_set_success++;
    }
    return return_value;

}
int kv_delete(char* key)
{
    switch(kv_replacement_policy)
    {
        case LRU:
            return lru_kv_delete(&lru_kv_store, key);

        default:
            return lru_kv_delete(&lru_kv_store, key);
    }
}


char** most_popular_keys(int k)
{
    char** popular = xmalloc(sizeof(popular)*k);
    int i;
    int n_entries = lru_kv_store.n_entries;
    if (kv_replacement_policy == LRU)
    {
        for(i=0; i<k; i++)
        {
            popular[i] = lru_kv_store.entries[n_entries-k].key;
        }
    }

    return popular;
}

int get_cache_info(char* kind)
{
    if (0 == strncmp(kind, "total_hits", strlen("total_hits")))
    {
        return kv_stats.total_hits;
    }
    else if (0 == strncmp(kind, "total_accesses", strlen("total_accesses")))
    {
        return kv_stats.total_accesses;
    }
    else if (0 == strncmp(kind, "total_set_success", strlen("total_set_success")))
    {
        return kv_stats.total_set_success;
    }
    else if (0 == strncmp(kind, "cache_size", strlen("cache_size")))
    {
        return kv_stats.cache_size;
    }
    else if (0 == strncmp(kind, "num_keys", strlen("num_keys")))
    {
        return kv_stats.num_keys;
    }
    else if (0 == strncmp(kind, "total_evictions", strlen("total_evictions")))
    {
        return kv_stats.total_evictions;
    }
    return -1;
}

void kv_reset()
{
    kv_stats = (kv_stats_t) {
        .total_hits=0,
        .total_accesses=0,
        .total_set_success=0,
        .cache_size=0,
        .num_keys=0,
        .total_evictions=0
    };
    if (kv_replacement_policy == LRU)
    {
        string_pair_table_destroy(&lru_kv_store);
    }
}