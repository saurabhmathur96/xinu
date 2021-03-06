#define KV_STORE_SIZE 500


typedef struct {
    char* key;
    char* value;
} string_pair_t;



typedef struct {
    string_pair_t *entries;
    int size;
    int n_entries;
} string_pair_table_t;

string_pair_table_t lru_kv_store;

typedef struct {
    string_pair_table_t t1;
    string_pair_table_t b1;
    string_pair_table_t t2;
    string_pair_table_t b2;
    int p;
} arc_store_t;

arc_store_t arc_kv_store;

typedef struct {
    int total_hits;
    int total_accesses;
    int total_set_success;
    int cache_size;
    int num_keys;
    int total_evictions;
} kv_stats_t;

kv_stats_t kv_stats;

typedef enum { LRU, ARC } replacement_policy_t;

replacement_policy_t kv_replacement_policy;

int kv_init(replacement_policy_t policy);
char* kv_get(char* key);
int kv_set(char* key, char* value);
int kv_delete(char* key);
void kv_reset();
int get_cache_info(char* kind);
char** most_popular_keys(int k);