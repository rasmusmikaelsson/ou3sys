/**
 * worker.h - Header for worker thread and path processing functions.
 * Defines the worker thread loop and the function to process file and directory tasks.
 * 
 * Author: Rasmus Mikaelsson (et24rmn)
 * Version: 13-11-2025
 */

#ifndef WORKER_H
#define WORKER_H

#include "system.h"


/**
 * worker - Worker thread function.
 * @args: Pointer to System struct.
 *
 * Return: NULL.
 */
void *worker(void *args);

/**
 * process_path - Handles a file or directory task.
 * @system: Pointer to System struct.
 * @task: Task containing path and sum pointer.
 *
 * Return: void.
 */
int process_path(System *system, Task *path);

#endif
