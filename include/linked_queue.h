typedef struct {
    pid32 value;
    linked_queue_node_t* next;
} linked_queue_node_t;

typedef struct {
    linked_queue_node_t* head;
    linked_queue_node_t* tail;
} linked_queue_t;
