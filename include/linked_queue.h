typedef struct linked_queue_node_s linked_queue_node_t;
struct linked_queue_node_s {
    pid32 value;
    linked_queue_node_t* next;
};

typedef struct {
    linked_queue_node_t* head;
    linked_queue_node_t* tail;
} linked_queue_t;


void linked_queue_initialize(linked_queue_t *q);

void linked_queue_destroy(linked_queue_t *q);

int linked_queue_is_empty(linked_queue_t *q);

void linked_queue_insert(linked_queue_t *q, pid32 value);

pid32 linked_queue_remove(linked_queue_t *q);
