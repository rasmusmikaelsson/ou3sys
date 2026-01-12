/**
 * queue.h - Generic FIFO queue implementation.
 *
 * Provides a dynamically allocated first-in-first-out queue with basic
 * operations for enqueueing, dequeueing, and inspecting elements.
 * The queue stores generic pointers and does not manage element memory.
 *
 * Author: Rasmus Mikaelsson (et24rmn)
 * Version: 13-11-2025
 */

#ifndef QUEUE_H
#define QUEUE_H

#include <stdbool.h>

typedef struct Queue Queue;

/**
 * Creates a new, empty queue.
 *
 * @return		Created, empty queue
 */
Queue *create_queue(void);

/**
 * Gets the size of a given queue
 *
 * @param q		Pointer to queue
 * @return		Size of queue
 */
int size(Queue *q);

/**
 * Checks if a queue is empty or not.
 *
 * @param q		Pointer to queue
 * @return		True if empty, false oterwise
 */
bool is_empty(Queue *q);

/**
 * Gets first item in the queue.
 *
 * @param q			Queue pointer
 * @return			Element at the front of the queue, -1 if queue is empty
 */
void *peek(Queue *q);

/**
 * Add item to queue
 *
 * @param q		Queue pointer
 * @param item	Item to add to queue
 * @return		0 on success, -1 if queue is full
 */
int enqueue(Queue *q, void *item);

/**
 * Removes item first in line from queue.
 *
 * @param q			Queue pointer
 * @param status	TODO
 * @return
 */
void *dequeue(Queue *q);

/**
 * frees all the queues allocated memory on the heap
 *
 * @param q		Pointer to queue to be destroyed
 */
void free_queue(Queue *q);

#endif

