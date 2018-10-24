#include <xinu.h>

void xmalloc_init(void)
{
    // call mkbufpool
    int i, j;
    int buffer_pool_size;
    bpid32 buffer_pool_id;
    for (i=0, buffer_pool_size = 8 /* start with 2^3 */; 
         i<N_BUFFER_POOLS;
         i++, buffer_pool_size *= 2 /* powers of 2 till 2^N_BUFFER_POOLS*/)
    {
        buffer_pool_id = mkbufpool(buffer_pool_size, N_BUFFERS);
        if (buffer_pool_id < 0)
        {
            // Error
            printf("Error creating buffer pool of size %d\n", buffer_pool_size);
            return;
        }
        buffer_pools[i] = (buffer_pool_entry_t){ .id = buffer_pool_id, 
                                                 .buffer_size = buffer_pool_size, 
                                                 .n_allocated = 0 };
        for (j=0; j<N_BUFFERS; j++)
        {
            buffer_pools[i].allocated_buffers[j] = (buffer_entry_t) { .buffer = NULL, 
                                                                      .length = -1 };
        }
    }
}
void* xmalloc(unsigned int size)
{
    int i = 0;
    for (i=0; i<N_BUFFER_POOLS /* traverse from 0 to N_BUFFER_POOLS */
           && !(buffer_pools[i].n_allocated == N_BUFFERS) /* skipping buffer pools that are full */
           && size > buffer_pools[i].buffer_size; /* pass over buffer pools with block size smaller than requested size */
        i++) {}
    if (i == N_BUFFER_POOLS)
    {
        // requested size larger than largest available buffer pool
        // return null
        return NULL;
    }

    // requested size >= block size of buffer pool at ith index

    char *buffer = getbuf(buffer_pools[i].id);
    if (!buffer)
    {
        // error removing buffer from buffer pool
        return NULL;
    }

    // update buffer pool table
    int j;
    for (j=0; j<N_BUFFERS /* traverse from 0 to N_BUFFERS */
          && !(buffer_pools[i].allocated_buffers[j].buffer == NULL); /* find first empty space */ 
        j++) {}
    
    if (j == N_BUFFERS)
    {
        // No free space found
        // => allocated_buffers is in inconsistent state
        freebuf(buffer);
        return NULL;
    }
    buffer_pools[i].n_allocated++;
    buffer_pools[i].allocated_buffers[j] = (buffer_entry_t) { .buffer = buffer, 
                                                              .length = size };
    
    return buffer;
}
void xfree(void* p)
{
    char *buffer = (char*) p;
    int i, j;
    for (i=0; i<N_BUFFER_POOLS; i++)
    {
        for (j=0; j<N_BUFFERS; j++)
        {
            if (buffer == buffer_pools[i].allocated_buffers[j].buffer)
            {
                // update buffer pool table
                buffer_pools[i].n_allocated--;
                // remove buffer from allocated list
                buffer_pools[i].allocated_buffers[j] = (buffer_entry_t) { .buffer = NULL, 
                                                                          .length = -1 };
                freebuf(buffer);
            }
        }
    }

    // buffer not found in buffer pool table
    // error
    printf ("Buffer not recognized.\n");
}

char* heap_snapshot()
{
    char message[2048];
    // pool_id=1, buffer_size=32, total_buffers=20, allocated_bytes=243, allocated_buffers=10, fragmented_bytes=77
    int i, j;
    int allocated_bytes;
    int fragmented_bytes;

    for (i=0; i<N_BUFFER_POOLS; i++)
    {
        allocated_bytes = 0;
        for (j=0; j<N_BUFFERS 
              && !(buffer_pools[i].allocated_buffers[j].buffer == NULL); /* skip over empty buffer entries */ 
             j++)
        {
            allocated_bytes += buffer_pools[i].allocated_buffers[j].length;

        }
        fragmented_bytes = buffer_pools[i].n_allocated * buffer_pools[i].buffer_size - allocated_bytes;
        sprintf(message + strlen(message), "pool_id=%d, buffer_size=%d, total_buffers=%d, allocated_bytes=%d, allocated_buffers=%d, fragmented_bytes=%d\n",
                buffer_pools[i].id, buffer_pools[i].buffer_size, N_BUFFERS, allocated_bytes, buffer_pools[i].n_allocated, fragmented_bytes);
    }
}