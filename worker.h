#ifndef WORKER_H
#define WORKER_H

#include "system.h"


/**
 * worker - Thread worker function.
 * @args: Pointer to System struct.
 *
 * Return: NULL.
 */
void *worker(void *args);

/**
 * process_path - Handle a file or directory task.
 * @system: Pointer to System struct.
 * @path: Path to file or directory.
 *
 * Return: void.
 */
void process_path(System *system, Task *path);

/**
 * process_directory - Reads directory entries and enqueues new tasks.
 * @system: Pointer to System struct.
 * @path: Directory path.
 *
 * Return: 0 on success, -1 on error.
 */
int process_directory(System *system, const char *path);

#endif
