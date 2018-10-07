/* future.c - future implementation */

#include <xinu.h>

future_t* future_alloc(future_mode_t mode)
{
    future_t *f = getmem(sizeof(*f));
    if (!f)
    {
        return NULL;
    }
    f->state = FUTURE_EMPTY;
    f->mode = mode;
    linked_queue_initialize(f->get_queue);
    linked_queue_initialize(f->set_queue);
    return f;
}
syscall future_free(future_t* f)
{
    intmask mask = disable();
    freemem(f, sizeof(*f));
    restore(mask);
    return OK;
}
syscall future_get(future_t* f, int* value)
{
    intmask mask = disable();
    if (f->state == FUTURE_READY)
    {
        // currently set value is available for use
        value = f->value;
    }
    else if (f->state == FUTURE_EMPTY)
    {
        // no value available
        // wait in queue
        linked_queue_insert(f->get_queue, currpid);

        // wakeup
        value = f->value;
    }
    else if (f->state == FUTURE_WAITING)
    {
        if (f->mode == FUTURE_EXCLUSIVE)
        {
            // More than one process calling future_get on an exclusive future
            restore(mask);
            return SYSERR;
        }
        // no value available
        // wait in queue
        linked_queue_insert(f->get_queue, currpid);

        // wakeup
        value = f->value;
    }
    
    if (f->mode == FUTURE_QUEUE)
    {
        f->state = FUTURE_EMPTY;
    }
    restore(mask);
    return OK;
}

syscall future_set(future_t* f, int value)
{
    intmask mask = disable();
    
    if (f->state == FUTURE_EMPTY)
    {
        f->value = value;
        f->state = FUTURE_READY;
    }
    else if (f->state == FUTURE_WAITING)
    {
        f->value = value;
        f->state = FUTURE_READY;
        pid32 get_pid = linked_queue_remove(f->get_queue);
        // wakeup get_pid process
    } 
    else if (f->state == FUTURE_READY)
    {
        if (f->mode == FUTURE_EXCLUSIVE || f->mode == FUTURE_SHARED)
        {
            // multiple calls to future_set not allowed
            restore(mask);
            return SYSERR;
        }
        // there is already an unread value set
        pid32 current_pid = currpid;
        linked_queue_insert(f->set_queue, current_pid);

        // suspend self

        // on wakeup
        f->value = value;
        f->state = FUTURE_READY;
    }
    
    restore(mask);
    return OK;
}
 