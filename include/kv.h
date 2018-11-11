#define KV_STORE_SIZE 100


typedef struct {
    char* key;
    char* value;
} string_pair_t;



typedef struct {
    string_pair_t *entries;
    int size;
    int n_entries;
} string_pair_table_t;

string_pair_table_t kv_store;

typedef struct {
    string_pair_table_t t1;
    string_pair_table_t b1;
    string_pair_table_t t2;
    string_pair_table_t b2;
    int p;

    int total_hits;
    int total_accesses;
    int total_set_success;
    int cache_size;
    int num_keys;
    int total_evictions;
} arc_cache_t;

int kv_init();
char* kv_get(char* key);
int kv_set(char* key, char* value);
bool kv_delete(char* key);
void kv_reset();
int get_cache_info(char* kind);
char** most_popular_keys(int k);