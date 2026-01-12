/**
 * worker.c - Implements worker threads for processing file system tasks.
 *
 * This file defines the worker thread loop and path processing functions.
 * It handles files and directories, sums file blocks, and avoids double-counting
 * inodes using a seen-inode list.
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

/* ------------------ Declarations of internal functions ------------------ */

static int *fail_code(void);
static int *critcal_fail_code(void);
static int unlock_mutex(pthread_mutex_t *m);
static int lock_mutex(pthread_mutex_t *m);
static int wait_cond(pthread_cond_t *cond, pthread_mutex_t *lock);

/* -------------------------- External functions -------------------------- */

int process_path(System *system, Task *task) {
    struct stat sb;
    if (lstat(task->path, &sb) == -1) {
        perror("lstat");
        return -1;
    }

    /* Lock mutex */
	if(lock_mutex(system->lock) != 0) {
		return -1;
	}

    /* Update sum */
    *(task->sum) += sb.st_blocks;
	
    /* Unlock mutex */
    if(unlock_mutex(system->lock) != 0) {
		return -1;
	}

    /* If path is a directory, enqueue child tasks */
    if (S_ISDIR(sb.st_mode)) {
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

            Task *child_task = malloc(sizeof(Task));
            child_task->path = malloc(PATH_MAX);
            if(!child_task || !child_task) {
                perror("malloc");
                free(child_task);
                return -1;
            }

			child_task->sum = task->sum;
            snprintf(child_task->path, PATH_MAX, "%s/%s", task->path, entry->d_name);
            if(system_enqueue(system, child_task) != 0) {
                free(child_task->path);
                free(child_task);
                closedir(dir);
                return -1;
            }
        }
        closedir(dir);
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
        free(task->path);
        free(task);
    }
}

/* -------------------------- Internal functions -------------------------- */

static int *fail_code(void) {
    int *failed = malloc(sizeof(int));
    *failed = -1;

    return failed;
}

static int *critcal_fail_code(void) {
    int *failed = malloc(sizeof(int));
    *failed = -2;

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
