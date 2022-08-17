#ifndef QUEUE_H
#define QUEUE_H

#define QUEUE_CAPACITY 124
#define QUEUE_SIZE (QUEUE_CAPACITY + 1)

struct queue {
  void **dequeue_pos;
  void **enqueue_pos;
  long id;
  void *data[QUEUE_SIZE];
};

void queue_init(struct queue *q, long id);
long enqueue(struct queue *q, void *ptr);
void *dequeue(struct queue *q);

#endif