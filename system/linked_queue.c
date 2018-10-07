/* linked_queue.c - linked_queue */

#include <xinu.h>

void linked_queue_initialize(linked_queue_t *q)
{
    q->head = q->tail = NULL;
}

void linked_queue_destroy(linked_queue_t *q)
{
    q->head = q->tail = NULL;
}

int linked_queue_is_empty(linked_queue_t *q)
{
    return q->head == NULL;
}

void linked_queue_insert(linked_queue_t *q, pid32 value)
{
    linked_queue_node_t *node = getmem(sizeof(*node));
    if (!node)
    {
        printf("Error: unable to allocate memory.\n");
    }

    node->value = value;
    node->next = NULL;

    if (q->tail == NULL)
    {
        q->tail = q->head = node;
    }
    else
    {
        q->tail->next = node;
        q->tail = node;
    }
}

pid32 linked_queue_remove(linked_queue_t *q)
{
    linked_queue_node_t *node = q->head;
    if (!node)
    {
        // Empty queue
        printf("Error: Queue empty.\n");
        return -1;
    }

    if (q->head == q->tail)
    {
        // Last element
        q->head = q->tail = NULL;
    }
    else
    {
        q->head = q->head->next;
    }
    pid32 value = node->value;
    freemem(node, sizeof(*node));
    return value;
}


