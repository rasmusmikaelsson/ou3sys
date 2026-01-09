/**
 * worker.c - Implements worker threads for processing file system tasks.
 *
 * This file defines the worker thread loop and path processing functions.
 * It handles files and directories, sums file blocks, and avoids double-counting
 * inodes using a seen-inode list.
 * 
 * Author: Rasmus Mikaelsson (et24rmn)
 * Version: 13-11-2025
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

static bool inode_seen(System *sys, dev_t st_dev, ino_t st_ino);
static int *fail_code(void);

/**
 * process_path - Handles path: adds file blocks or explores directory.
 * @system: Pointer to System struct.
 * @path: Path to process.
 */
int process_path(System *system, Task *task) {
    struct stat sb;
    if (lstat(task->path, &sb) == -1) {
        perror("lstat");
        return -1;
    }

    //* Check if inode has already been counted */
    if (!inode_seen(system, sb.st_dev, sb.st_ino)) {
        pthread_mutex_lock(system->lock);
        *(task->sum) += sb.st_blocks;
        pthread_mutex_unlock(system->lock);
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
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
                continue;

            Task *child_task = malloc(sizeof(Task));
            child_task->path = malloc(PATH_MAX);
			child_task->sum = task->sum;
            snprintf(child_task->path, PATH_MAX, "%s/%s", task->path, entry->d_name);
            system_enqueue(system, child_task);
        }
        closedir(dir);
    }

	return 0;
}

static int *fail_code(void) {
    int *failed = malloc(sizeof(int));
    *failed = -1;

    return failed;
}

/**
 * worker - Worker thread loop for processing tasks.
 * @args: Pointer to System struct.
 *
 * Dequeues tasks from the system queue, processes paths, and frees task memory.
 * Terminates when the queue is empty and the system is marked done.
 */
void *worker(void *args) {
    System *system = (System *)args;

    /* Wait until queue has tasks or system is done */
    while (1) {
        pthread_mutex_lock(system->lock);
        while (is_empty(system->queue) && *(system->done) == 0) {
            pthread_cond_wait(system->cond, system->lock);
        }

        /* Exit if done and queue is empty */
        if (*(system->done) == 1 && is_empty(system->queue)) {
            pthread_mutex_unlock(system->lock);
            return NULL;
        }

        Task *task = dequeue(system->queue);
        pthread_mutex_unlock(system->lock);

        /* Process task: sum file blocks or enqueue directories */
        if(process_path(system, task) != 0) {
            free(task->path);
            free(task);

            return fail_code();
        }

        /* Free task memory */
        free(task->path);
        free(task);
    }
}

/**
 * inode_seen - Checks if an inode has already been processed.
 * @sys: Pointer to System struct.
 * @st_dev: Device ID of the file.
 * @st_ino: Inode number of the file.
 *
 * Return: true if inode was already seen, false otherwise.
 * Adds inode to the seen list if not already present.
 */
static bool inode_seen(System *sys, dev_t st_dev, ino_t st_ino)
{
    pthread_mutex_lock(&sys->inode_lock);

    Inode *current = sys->seen_inodes;
    while(current != NULL) {
        if(current->st_dev == st_dev && current->st_ino == st_ino) {
            pthread_mutex_unlock(&sys->inode_lock);
            return true;
        }
        current = current->next;
    }

    /* Add new inode to the seen list */
    Inode *node = malloc(sizeof(Inode));
    node->st_dev = st_dev;
    node->st_ino = st_ino;
    node->next = sys->seen_inodes;
    sys->seen_inodes = node;

    pthread_mutex_unlock(&sys->inode_lock);
    return false;
}



