
#define N_BUFFERS 10
#define N_BUFFER_POOLS 10

typedef struct {
    char *buffer; /* address of buffer */
    int32 length; /* actual length */
} buffer_entry_t;

typedef struct {
    bpid32 id;
    int32 buffer_size; /* allocated size of each buffer in pool */
    int32 n_allocated; /* number of buffers in use */
    buffer_entry_t allocated_buffers[N_BUFFERS];
} buffer_pool_entry_t;

buffer_pool_entry_t buffer_pools[N_BUFFER_POOLS];
int32 n_buffer_pools;
char heap_snapshot_buffer[2048];

void xmalloc_init(void);
void* xmalloc(unsigned int);
void xfree(void*);
char* heap_snapshot(void);