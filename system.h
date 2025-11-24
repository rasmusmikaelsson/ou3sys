/**
 * system.h - Header for system management in multithreaded file block counting.
 *
 * Defines the System struct, Task struct, and related functions
 * for thread synchronization, task queue management, and inode tracking.
 * 
 * Author: Rasmus Mikaelsson (et24rmn)
 * Version: 13-11-2025
 */

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

typedef struct Task {
    char *path;
    blkcnt_t *sum;
} Task;

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
 * Enqueue's a new task
 *
 * @system: Pointer to System struct
 * @task: Task to enqueue
 *
 * Returns: 0 on success, -1 on failure
 */
int system_enqueue(System *system, Task *task);

/**
 * Mark threads as done and joins them
 *
 * @system: Pointer to System struct
 * @threads: Thread array
 * @n_threads: Number of threads
 *
 * Returns: 0 on success, -1 of failure
 */
int system_join(System *system, pthread_t *threads, int n_threads);

/**
 * Initialize System struct and create worker threads
 *
 * @system: Pointer to System
 * @threads: Pre-allocated array of pthread_t
 * @n_threads: Number of threads
 *
 * Returns: 0 on success, -1 on failure
 */
int system_init(System *system, pthread_t *threads, int n_threads);

/**
 * Frees dynamically allocated fields inside System
 *
 * @system: Pointer to System struct
 *
 * Returns: 0 on success, -1 on failure
 */
int system_destroy(System *system);

#endif

