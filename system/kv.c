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

int lru_kv_set(string_pair_table_t* store, kv_stats_t* stats, char* key, char* value)
{
    if (string_pair_table_is_full(store)) 
    {
        string_pair_t pair = remove_string_pair(store,  0);
        stats->cache_size -= (strlen(pair.key) + strlen(pair.value));
        xfree(pair.key);
        xfree(pair.value);
        stats->total_evictions += 1;
    }
    

    // check if key is already in cache
    int i = find_string_pair(store, key);
    if (i >= 0) 
    {
        // found => update value and set as most recently used
        string_pair_t pair = remove_string_pair(store,  i);
        stats->cache_size += (strlen(value) - strlen(pair.value)) ;
        xfree(pair.value);

        return insert_back_string_pair(store, pair.key, duplicate_string(value));
    }

    // not found 
    stats->cache_size += (strlen(key) + strlen(value));
    stats->num_keys += 1;
    return insert_back_string_pair(store, duplicate_string(key), duplicate_string(value));
}

char* lru_kv_get(string_pair_table_t* store, kv_stats_t* stats, char* key)
{
    stats->total_accesses += 1;
    int target = find_string_pair(store, key);
    if (target<0)
    {
        return NULL;
    }
    stats->total_hits += 1;
    string_pair_t pair = remove_string_pair(store, target);
    insert_back_string_pair(store, pair.key, pair.value);
    return pair.value;
}

int lru_kv_delete(string_pair_table_t* store, kv_stats_t* stats , char* key)
{
    int index = find_string_pair(store, key);
    if (index < 0)
    {
        return 0;
    }
    string_pair_t pair = remove_string_pair(store, index);
    stats->cache_size -= (strlen(pair.key) + strlen(pair.value));
    stats->num_keys -= 1;
    xfree(pair.key);
    xfree(pair.value);
    return 1;
}


/**
 * ARC Implementation
 */

int arc_kv_set(char* key, char* value)
{
    if (string_pair_table_is_full(&(arc_kv_store.t1)))
    {
        // t1 is full => evict from t1 and  add to b1
        string_pair_t pair = remove_string_pair(&(arc_kv_store.t1),  0);
        kv_stats_t stats = (kv_stats_t) { .total_hits=0, .total_accesses=0, .total_set_success=0, 
                                            .cache_size=0, .num_keys=0, .total_evictions=0 };
        lru_kv_set(&(arc_kv_store.b1), &stats, pair.key, pair.value);
        kv_stats.total_evictions += stats.total_evictions;

    }

    int i;
    // check if key is already in t1
    i = find_string_pair(&(arc_kv_store.t1), key);
    if (i >= 0) 
    {
        // found => update value
        string_pair_t pair = remove_string_pair(&(arc_kv_store.t1),  i);
        kv_stats.cache_size += (strlen(value) - strlen(pair.value));
        xfree(pair.value);

        return insert_back_string_pair(&(arc_kv_store.t1), pair.key, duplicate_string(value));
    }

    // check if key is already in t2
    i = find_string_pair(&(arc_kv_store.t2), key);
    if (i >= 0) 
    {
        // found => update value
        string_pair_t pair = remove_string_pair(&(arc_kv_store.t2),  i);
        kv_stats.cache_size += (strlen(value) - strlen(pair.value));
        xfree(pair.value);

        return insert_back_string_pair(&(arc_kv_store.t2), pair.key, duplicate_string(value));
    }

    // check if key is already in b1
    i = find_string_pair(&(arc_kv_store.b1), key);
    if (i >= 0) 
    {
        // found => update value, add to t1
        string_pair_t pair = remove_string_pair(&(arc_kv_store.b1),  i);
        kv_stats.cache_size += (strlen(value) - strlen(pair.value));
        xfree(pair.value);

        return insert_back_string_pair(&(arc_kv_store.t1), pair.key, duplicate_string(value));
    }

    // check if key is already in b2
    i = find_string_pair(&(arc_kv_store.b2), key);
    if (i >= 0) 
    {
        // found => update value, add to t1
        string_pair_t pair = remove_string_pair(&(arc_kv_store.b2),  i);
        kv_stats.cache_size += (strlen(value) - strlen(pair.value));
        xfree(pair.value);

        return insert_back_string_pair(&(arc_kv_store.t1), pair.key, duplicate_string(value));
    }

    

    // not found in t1 or t2 => insert into t1
    kv_stats.cache_size += (strlen(key) + strlen(value));
    kv_stats.num_keys += 1;
    return insert_back_string_pair(&(arc_kv_store.t1), duplicate_string(key), duplicate_string(value));
}

char* arc_kv_get(char* key)
{
    int target, delta;
    // Look for key in t2
    target = find_string_pair(&(arc_kv_store.t2), key);
    if (target >= 0)
    {
        // Case 1: if found shift to MRU of t2
        string_pair_t pair = remove_string_pair(&(arc_kv_store.t2), target);
        if (string_pair_table_is_full(&(arc_kv_store.t2)))
        {
            // t2 is full => evict from t2 and  add to b2
            string_pair_t pair = remove_string_pair(&(arc_kv_store.t2),  0);
            kv_stats_t stats = (kv_stats_t) { .total_hits=0, .total_accesses=0, .total_set_success=0, 
                                            .cache_size=0, .num_keys=0, .total_evictions=0 };
            lru_kv_set(&(arc_kv_store.b2), &stats, pair.key, pair.value);
            kv_stats.total_evictions += stats.total_evictions;
        }
        insert_back_string_pair(&(arc_kv_store.t2), pair.key, pair.value);
        return pair.value;
    }

    // Look for key in t1
    target = find_string_pair(&(arc_kv_store.t1), key);
    if (target >= 0)
    {
        // Case 1: if found shift to MRU of t2
        string_pair_t pair = remove_string_pair(&(arc_kv_store.t1), target);
        if (string_pair_table_is_full(&(arc_kv_store.t2)))
        {
            // t2 is full => evict from t2 and  add to b2
            string_pair_t pair = remove_string_pair(&(arc_kv_store.t2),  0);
            kv_stats_t stats = (kv_stats_t) { .total_hits=0, .total_accesses=0, .total_set_success=0, 
                                            .cache_size=0, .num_keys=0, .total_evictions=0 };
            lru_kv_set(&(arc_kv_store.b2), &stats, pair.key, pair.value);
            kv_stats.total_evictions += stats.total_evictions;
        }
        insert_back_string_pair(&(arc_kv_store.t2), pair.key, pair.value);
        return pair.value;
    }

    // Look for key in b1
    target = find_string_pair(&(arc_kv_store.b1), key);
    if (target >= 0)
    {
        string_pair_t pair = remove_string_pair(&(arc_kv_store.b1), target);
        // shift pair retrieved from b1 to MRU of t2
        insert_back_string_pair(&(arc_kv_store.t2), pair.key, pair.value);


        // Case 2: compute p and call Replace(x_t, p)
        int b1 = arc_kv_store.b1.n_entries;
        int b2 = arc_kv_store.b2.n_entries;
        delta = b1 >= b2 ? 1 : (b2 / b1);

        int p = arc_kv_store.p;
        int c = arc_kv_store.t1.size;
        p = p+delta;
        p = p < c ? p : c; // min{p+delta, c}
        arc_kv_store.p = p;

        int t1_length = arc_kv_store.t1.n_entries;
        if (t1_length > 0 && t1_length > p) 
        {
            // evict from t1 and move to b1
            string_pair_t p = remove_string_pair(&(arc_kv_store.t1),  0);
            kv_stats_t stats = (kv_stats_t) { .total_hits=0, .total_accesses=0, .total_set_success=0, 
                                            .cache_size=0, .num_keys=0, .total_evictions=0 };
            lru_kv_set(&(arc_kv_store.b1), &stats, p.key, p.value);
        }
        else
        {
            // evict from t2 and move to b2
            string_pair_t p = remove_string_pair(&(arc_kv_store.t2),  0);
            
            kv_stats_t stats = (kv_stats_t) { .total_hits=0, .total_accesses=0, .total_set_success=0, 
                                            .cache_size=0, .num_keys=0, .total_evictions=0 };
            lru_kv_set(&(arc_kv_store.b2), &stats, p.key, p.value);
            kv_stats.total_evictions += stats.total_evictions;
        }

        
        return pair.value;
    }

    // Look for key in b2
    target = find_string_pair(&(arc_kv_store.b2), key);
    if (target >= 0)
    {

        // shift pair retrieved from b2 to MRU of t2
        string_pair_t pair = remove_string_pair(&(arc_kv_store.b2), target);
        insert_back_string_pair(&(arc_kv_store.t2), pair.key, pair.value);

        // Case 3: compute p and call Replace(x_t, p)
        int b1 = arc_kv_store.b1.n_entries;
        int b2 = arc_kv_store.b2.n_entries;
        delta = b2 >= b1 ? 1 : (b1 / b2);

        int p = arc_kv_store.p;
        p = p-delta;
        p = p > 0 ? p : 0; // max{p-delta, 0}
        arc_kv_store.p = p;
        
        int t1_length = arc_kv_store.t1.n_entries;
        if (t1_length > 0 && t1_length >= p)
        {
            // evict from t1 and move to b1
            string_pair_t p = remove_string_pair(&(arc_kv_store.t1),  0);
            kv_stats_t stats = (kv_stats_t) { .total_hits=0, .total_accesses=0, .total_set_success=0, 
                                            .cache_size=0, .num_keys=0, .total_evictions=0 };
            lru_kv_set(&(arc_kv_store.b1), &stats, p.key, p.value);
        }
        else
        {
            // evict from t2 and move to b2
            string_pair_t p = remove_string_pair(&(arc_kv_store.t2),  0);
            
            kv_stats_t stats = (kv_stats_t) { .total_hits=0, .total_accesses=0, .total_set_success=0, 
                                            .cache_size=0, .num_keys=0, .total_evictions=0 };
            lru_kv_set(&(arc_kv_store.b2), &stats, p.key, p.value);
            kv_stats.total_evictions += stats.total_evictions;
        }
        
        return pair.value;
    }

    // Not found anywhere
    
    return NULL;
}


int arc_kv_delete(char* key)
{
    kv_stats_t stats;
    int return_value;

    stats = (kv_stats_t) { .total_hits=0, .total_accesses=0, .total_set_success=0, 
                           .cache_size=0, .num_keys=0, .total_evictions=0 };
    return_value = lru_kv_delete(&(arc_kv_store.t1), &stats, key);
    if (return_value == 1)
    {
        kv_stats.cache_size += stats.cache_size;
        kv_stats.num_keys += stats.num_keys;
        return 1;
    }

    stats = (kv_stats_t) { .total_hits=0, .total_accesses=0, .total_set_success=0, 
                           .cache_size=0, .num_keys=0, .total_evictions=0 };
    return_value = lru_kv_delete(&(arc_kv_store.t2), &stats, key);
    if (return_value == 1)
    {
        kv_stats.cache_size += stats.cache_size;
        kv_stats.num_keys += stats.num_keys;
        return 1;
    }

    stats = (kv_stats_t) { .total_hits=0, .total_accesses=0, .total_set_success=0, 
                           .cache_size=0, .num_keys=0, .total_evictions=0 };
    return_value = lru_kv_delete(&(arc_kv_store.b1), &stats, key);
    if (return_value == 1)
    {
        kv_stats.cache_size += stats.cache_size;
        kv_stats.num_keys += stats.num_keys;
        return 1;
    }

    stats = (kv_stats_t) { .total_hits=0, .total_accesses=0, .total_set_success=0, 
                           .cache_size=0, .num_keys=0, .total_evictions=0 };
    return_value = lru_kv_delete(&(arc_kv_store.b2), &stats, key);
    if (return_value == 1)
    {
        kv_stats.cache_size += stats.cache_size;
        kv_stats.num_keys += stats.num_keys;
        return 1;
    }

    // Not found in any of the four lists
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
    else if (policy == ARC)
    {
        kv_replacement_policy = policy;
        string_pair_table_init(&(arc_kv_store.t1), KV_STORE_SIZE);
        string_pair_table_init(&(arc_kv_store.t2), KV_STORE_SIZE);
        string_pair_table_init(&(arc_kv_store.b1), KV_STORE_SIZE);
        string_pair_table_init(&(arc_kv_store.b2), KV_STORE_SIZE);
        arc_kv_store.p = 0;


    }
    return 0;
}

char* kv_get(char* key)
{
    char* return_value;
    switch(kv_replacement_policy)
    {
        case LRU:
            return_value = lru_kv_get(&lru_kv_store, &kv_stats, key);
            break;
        
        case ARC:
            return_value = arc_kv_get(key);
            break;

        default:
            return_value = lru_kv_get(&lru_kv_store, &kv_stats, key);
    }
    
    return return_value;
}

int kv_set(char* key, char* value)
{
    if (strlen(key) > 64 || strlen(value) > 1024)
    {
        return 1;
    }
    int return_value;
    switch(kv_replacement_policy)
    {
        case LRU:
            return_value = lru_kv_set(&lru_kv_store, &kv_stats, key, value);
            break;
        
        case ARC:
            return_value = arc_kv_set(key, value);
            break;

        default:
            return_value = lru_kv_set(&lru_kv_store, &kv_stats, key, value);
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
            return lru_kv_delete(&lru_kv_store, &kv_stats, key);
            break;
        
        case ARC:
            return arc_kv_delete(key);
            break;

        default:
            return lru_kv_delete(&lru_kv_store, &kv_stats, key);
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
    else if (kv_replacement_policy == ARC)
    {
        arc_kv_store.p = 0;
        string_pair_table_destroy(&(arc_kv_store.t1));
        string_pair_table_destroy(&(arc_kv_store.t2));
        string_pair_table_destroy(&(arc_kv_store.b1));
        string_pair_table_destroy(&(arc_kv_store.b2));
        
    }
}