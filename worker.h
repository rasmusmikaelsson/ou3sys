/**
 * worker.h - Header for worker thread and path processing functions.
 * Defines the worker thread loop and the function to process file and directory tasks.
 * 
 * Author: Rasmus Mikaelsson (et24rmn)
 * Version: 13-11-2025
 *          12-01-2026 (Current)
 */

#ifndef WORKER_H
#define WORKER_H

#include "system.h"

/**
 * process_path - Handles path: adds file blocks or explores directory.
 * @system: Pointer to System struct.
 * @path: Path to process.
 * 
 * @return 0 on success, otherwise -1
 */
int process_path(System *system, Task *path);

/**
 * worker - Worker thread routine that processes queued tasks until termination.
 * @args: Pointer to the system structure.
 *
 * Return: Thread exit status.
 */
void *worker(void *args);

#endif
