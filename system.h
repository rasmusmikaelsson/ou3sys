/**
 * system.h - Header for system management in multithreaded file block counting.
 *
 * Defines the System struct, Task struct, and related functions
 * for thread synchronization, and task queue management.
 * 
 * Author: Rasmus Mikaelsson (et24rmn)
 * Version: 13-11-2025
 */

#ifndef SYSTEM_H
#define SYSTEM_H

#include <pthread.h>
#include <sys/types.h>
#include "queue.h"

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
	int status;
} System;

/**
 * system_enqueue - Enqueues a task and signals a waiting worker thread.
 * @system: Pointer to the system structure.
 * @task: Pointer to the task to enqueue.
 *
 * Return: 0 on success, -1 on failure.
 */
int system_enqueue(System *system, Task *task);

/**
 * system_join - Signals worker threads to terminate and joins them.
 * @system: Pointer to the system structure.
 * @threads: Array of worker thread identifiers.
 * @n_threads: Number of worker threads.
 *
 * Return: 0 on success, -1 on failure.
 */
int system_join(System *system, pthread_t *threads, int n_threads);

/**
 * system_init - Initializes system resources and creates worker threads.
 * @system: Pointer to the system structure to initialize.
 * @threads: Array to store created worker thread identifiers.
 * @n_threads: Number of worker threads to create.
 *
 * Return: 0 on success, -1 on failure.
 */
int system_init(System *system, pthread_t *threads, int n_threads);

/**
 * system_destroy - Frees all system resources and destroys synchronization primitives.
 * @system: Pointer to the system structure to destroy.
 *
 * Return: 0 on success, -1 on failure.
 */
int system_destroy(System *system);

#endif
