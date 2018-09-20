#include "ring_buffer.h"

void ring_buffer_initialize(ring_buffer_t *b)
{
    b->head = b->tail = -1;
    pthread_mutex_init(&(b->access_lock), NULL);
    sem_init(&(b->n_empty), 0, 0);
    sem_init(&(b->n_full), 0, RING_BUFFER_SIZE);
}

void ring_buffer_destroy(ring_buffer_t *b)
{
    b->head = b->tail = -1;
    pthread_mutex_destroy(&(b->access_lock));
    sem_destroy(&(b->n_empty));
    sem_destroy(&(b->n_full));
}


int _ring_buffer_is_empty(ring_buffer_t *b)
{
    return (b->head == -1) && (b->head == b->tail);
}

int _ring_buffer_is_full(ring_buffer_t *b)
{
    return b->tail == (b->head+1)%RING_BUFFER_SIZE;
}

void ring_buffer_insert(ring_buffer_t *b, void *value)
{
    sem_wait(&(b->n_full)); // Wait if all places are full
    pthread_mutex_lock(&(b->access_lock));

    if (_ring_buffer_is_full(b))
    {
        // Error !
        printf("Buffer is full !\n");
    }
    // 1. Increment head
    if (_ring_buffer_is_empty(b))
    {
        // Case: buffer is empty
        b->head = b->tail = 0;
    }
    else
    {
        b->head = b->head + 1;
        if (b->head == RING_BUFFER_SIZE)
        {
            // Case: wrap around    
            b->head = 0;
        }
    }
    // 2. Assign element
    b->data[b->head] = value;
    
    pthread_mutex_unlock(&(b->access_lock));
    sem_post(&(b->n_empty)); // Signal that there are new elements
}

void *ring_buffer_remove(ring_buffer_t *b)
{
    sem_wait(&(b->n_empty)); // Wait if no elements to take
    pthread_mutex_lock(&(b->access_lock));
    

    if (_ring_buffer_is_empty(b))
    {
        // Error !
        printf("buffer is empty !\n");
    } 
    void * value = b->data[b->tail];
    if (b->head == b->tail)
    {
        // Case: last element
        b->head = b->tail = -1;
    }
    else 
    {
        b->tail = b->tail + 1;
        if (b->tail == RING_BUFFER_SIZE)
        {
            
            // Case: wrap around 
            b->tail = 0;
        }
    }
    

    pthread_mutex_unlock(&(b->access_lock));
    sem_post(&(b->n_full)); // Signal that buffer is not full
    
    return value;
}