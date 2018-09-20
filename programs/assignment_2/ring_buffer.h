/**
 * ring_buffer.h
 * 
 * A shared ring buffer implemented 
 * using a fixed size array
 * 
 */ 
#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>

#ifndef RING_BUFFER_H
#define RING_BUFFER_H

#define RING_BUFFER_SIZE 1024

typedef struct {
    // Data storage
    void* data[RING_BUFFER_SIZE];
    size_t head;
    size_t tail;
    
    // Synchronization
    pthread_mutex_t access_lock;
    sem_t n_empty;
    sem_t n_full;
} ring_buffer_t;

void ring_buffer_initialize(ring_buffer_t *b);
void ring_buffer_insert(ring_buffer_t *b, void *value);
void *ring_buffer_remove(ring_buffer_t *b);
void ring_buffer_destroy(ring_buffer_t *b);

#endif