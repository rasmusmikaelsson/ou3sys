#ifndef SYSTEM_H
#define SYSTEM_H

#include <pthread.h>
#include <sys/types.h>
#include "queue.h"

typedef struct Inode {
    dev_t st_dev;
    ino_t st_ino;
    struct Inode *next;
} Inode;

/**
 * struct System - Holds synchronization objects and shared program state.
 * @cond: Condition variable for worker synchronization.
 * @lock: Mutex protecting shared state.
 * @done: Flag indicating no more tasks will arrive.
 * @queue: Pointer to task queue.
 * @sum: Pointer to total block count.
 */
typedef struct System {
    pthread_cond_t *cond;
    pthread_mutex_t *lock;
    int *done;
    Queue *queue;
    blkcnt_t *sum;

    pthread_mutex_t inode_lock;
    Inode *seen_inodes;
} System;

/**
 * system_init - Initialize the System structure and start worker threads.
 * @system: Pointer to System struct.
 * @threads: Pointer to allocated pthread_t array.
 * @n_threads: Number of threads to create.
 *
 * Return: 0 on success, -1 on error.
 */
int system_init(System *system, pthread_t *threads, int n_threads);

/**
 * system_destroy - Clean up allocated resources inside System.
 * @system: Pointer to System struct.
 *
 * Return: 0 on success.
 */
int system_destroy(System *system);

/**
 * system_join - Marks thread work as finished and joins all workers.
 * @system: Pointer to System struct.
 * @threads: Pointer to thread array.
 * @n_threads: Number of threads.
 *
 * Return: 0 on success.
 */
int system_join(System *system, pthread_t *threads, int n_threads);

/**
 * system_enqueue - Thread-safe enqueue of a task string.
 * @system: Pointer to System struct.
 * @task: Path to enqueue (malloc'ed string).
 *
 * Return: void.
 */
void system_enqueue(System *system, void *task);

#endif
