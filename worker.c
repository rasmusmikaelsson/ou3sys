/**
 * worker.c - Worker thread logic for multithreaded file block counting.
 *
 * Contains the worker thread routine and helper functions for processing
 * filesystem paths, handling task execution, and reporting thread failure
 * states in a thread-safe manner.
 * 
 * Author: Rasmus Mikaelsson (et24rmn)
 * Version: 13-11-2025
 *          12-01-2026 (Current)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <linux/limits.h>
#include <sys/stat.h>
#include <dirent.h>
#include <stdbool.h>

#include "worker.h"
#include "system.h"
#include "queue.h"

#define MIN(a, b) ((a) < (b) ? (a) : (b))

/* ------------------ Declarations of internal functions ------------------ */

static int *fail_code(void);
static int *critcal_fail_code(void);
static int unlock_mutex(pthread_mutex_t *m);
static int lock_mutex(pthread_mutex_t *m);
static int wait_cond(pthread_cond_t *cond, pthread_mutex_t *lock);

/* -------------------------- External functions -------------------------- */

int process_path(System *system, Task *task) {
    blkcnt_t size = 0;
    char path[PATH_MAX];

    DIR *dir = opendir(task->path);
    if (!dir) {
        perror("opendir");
        return -1;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        memcpy(path, task->path, strlen(task->path));
        path[strlen(task->path)] = '/';
        memcpy(path + strlen(task->path) + 1, entry->d_name, strlen(entry->d_name)+1);
        struct stat sb;
        if (lstat(path, &sb) == -1) {
            perror("lstat");
            if(closedir(dir) != 0) {
                perror("closedir");
            }
            return -1;
        }
        size += sb.st_blocks;

        if (S_ISDIR(sb.st_mode)) {
            Task *child_task = malloc(sizeof(Task));
            if(!child_task) {
                perror("malloc");
                return -1;
            }

            child_task->sum = task->sum;
            strncpy(child_task->path, path, PATH_MAX);
            if(system_enqueue(system, child_task) != 0) {
                free(child_task);
                if(closedir(dir) != 0) {
                    perror("closedir");
                }
                return -1;
            }
        }

    }
    if(closedir(dir) != 0 ) {
        perror("closedir");
        return -1;
    }
    
    /* Lock mutex */
	if(lock_mutex(system->lock) != 0) {
		return -1;
	}

    /* Update sum */
    *(task->sum) += size;
	
    /* Unlock mutex */
    if(unlock_mutex(system->lock) != 0) {
		return -1;
	}
	return 0;
}

void *worker(void *args) {
    System *system = (System *)args;
    int status = 0;

    /* Wait until queue has tasks or system is done */
    while (1) {
        if(lock_mutex(system->lock) != 0) {
            return critcal_fail_code();
        }

        while (is_empty(system->queue) && *(system->done) == 0) {
            if(wait_cond(system->cond, system->lock) != 0) {
                return critcal_fail_code();
            }
        }

        /* Exit if done and queue is empty */
        if (*(system->done) == 1 && is_empty(system->queue)) {
            if(unlock_mutex(system->lock) != 0) {
                return critcal_fail_code();
            }
            return status == 0 ? NULL : fail_code();
        }

        Task *task = dequeue(system->queue);
        if(unlock_mutex(system->lock) != 0) {
            return critcal_fail_code();
        }

        /* Process task: sum file blocks or enqueue directories */
        if(process_path(system, task) != 0) {
            status = -1;
        }

        /* Free task memory */
        free(task);
    }
}

/* -------------------------- Internal functions -------------------------- */

/**
 * fail_code - Allocates and returns a non-critical failure code.
 *
 * Returns: Pointer to an integer representing a recoverable thread failure.
 */
static int *fail_code(void) {
    int *failed = malloc(sizeof(int));
    *failed = 1;

    return failed;
}

/**
 * critcal_fail_code - Allocates and returns a critical failure code.
 *
 * Returns: Pointer to an integer representing a critical thread failure.
 */
static int *critcal_fail_code(void) {
    int *failed = malloc(sizeof(int));
    *failed = 2;

    return failed;
}

/**
 * Unlocks the mutex
 * @m: Pointer to the mutex to unlock
 *
 * Returns: 0 on success, -1 on failure
 */
static int unlock_mutex(pthread_mutex_t *m) {
    int ret = pthread_mutex_unlock(m);
    if (ret != 0) {
        fprintf(stderr, "pthread_mutex_unlock failed: %s\n", strerror(ret));
        return -1;
    }
    return 0;
}

/**
 * Locks the mutex
 * @m: Pointer to the mutext to lock
 * 
 * Returns: 0 on success, -1 on failure
 */
static int lock_mutex(pthread_mutex_t *m) {
    int ret = pthread_mutex_lock(m);
    if(ret != 0) {
        fprintf(stderr, "pthread_mutext_lock failed: %s\n", strerror(ret));
        return -1;
    }
    return 0;
}

/**
 * wait_cond - Waits on a condition variable.
 * @cond: Pointer to the condition variable to wait on.
 * @lock: Pointer to the associated mutex.
 *
 * Return: 0 on success, -1 on failure.
 */
static int wait_cond(pthread_cond_t *cond, pthread_mutex_t *lock) {
    int ret = pthread_cond_wait(cond, lock);
    if (ret != 0) {
        fprintf(stderr, "pthread_cond_wait failed: %s\n", strerror(ret));
        return -1;
    }
    return 0;
}
