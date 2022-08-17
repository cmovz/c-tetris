#include "queue.h"
#include "barrier.h"
#include <unistd.h>

void queue_init(struct queue *q, long id)
{
  q->dequeue_pos = q->data;
  q->enqueue_pos = q->data;
  q->id = id;
}

long enqueue(struct queue *q, void *ptr)
{
  barrier();
  void **next = q->enqueue_pos + 1;
  if (next == q->data + QUEUE_SIZE)
    next = q->data;

  if (next == q->dequeue_pos)
    return -1;
  
  *q->enqueue_pos = ptr;
  barrier();
  q->enqueue_pos = next;
  return 0;
}

void *spin_dequeue(struct queue *q)
{
  barrier();
  while (q->dequeue_pos == q->enqueue_pos) {
    usleep(0);
    barrier();
  }

  void **next = q->dequeue_pos + 1;
  if (next == q->data + QUEUE_SIZE)
    next = q->data;
  
  void *ptr = (void*)*q->dequeue_pos;
  barrier();
  q->dequeue_pos = next;

  return ptr;
}