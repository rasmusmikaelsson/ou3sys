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

/**
 * process_directory - Reads directory entries and enqueues new paths.
 * @system: Pointer to System struct.
 * @path: Directory path.
 *
 * Return: 0 on success, -1 on error.
 */
int process_directory(System *system, const char *path)
{
    DIR *dir = opendir(path);
    if (!dir) {
        perror("opendir");
        return -1;
    }

    struct dirent *entry;

    /* Loop through directory entries */
    while ((entry = readdir(dir)) != NULL) {
        /* Skip . and .. */
        if (strcmp(entry->d_name, ".") == 0 ||
            strcmp(entry->d_name, "..") == 0)
            continue;

        /* Create task for next path */
        Task *child_task = malloc(sizeof(Task));
        child_task->path = malloc(PATH_MAX);
        snprintf(child_task->path, PATH_MAX, "%s/%s", path, entry->d_name);
        child_task->sum = 0;

        system_enqueue(system, child_task);
    }

    closedir(dir);
    return 0;
}

/**
 * process_path - Handles path: adds file blocks or explores directory.
 * @system: Pointer to System struct.
 * @path: Path to process.
 */
void process_path(System *system, Task *task) {
    struct stat sb;
    if (lstat(task->path, &sb) == -1) {
        perror("lstat");
        return;
    }

    // Kolla inode-set
    if (!inode_seen(system, sb.st_dev, sb.st_ino)) {
        pthread_mutex_lock(system->lock);
        task->sum += sb.st_blocks;  // summera lokalt för denna task
        pthread_mutex_unlock(system->lock);
    }

    if (S_ISDIR(sb.st_mode)) {
        DIR *dir = opendir(task->path);
        if (!dir) return;

        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL) {
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
                continue;

            Task *child_task = malloc(sizeof(Task));
            child_task->path = malloc(PATH_MAX);
            snprintf(child_task->path, PATH_MAX, "%s/%s", task->path, entry->d_name);
            child_task->sum = 0;
            system_enqueue(system, child_task);
        }
        closedir(dir);
    }
}


/**
 * worker - Thread loop for processing tasks.
 * @args: Pointer to System struct.
 */
void *worker(void *args) {
    System *system = (System *)args;

    while (1) {
        pthread_mutex_lock(system->lock);
        while (is_empty(system->queue) && *(system->done) == 0) {
            pthread_cond_wait(system->cond, system->lock);
        }
        if (*(system->done) == 1 && is_empty(system->queue)) {
            pthread_mutex_unlock(system->lock);
            return NULL;
        }

        Task *task = dequeue(system->queue);
        pthread_mutex_unlock(system->lock);

        // Bearbeta task: summera filer/kataloger
        process_path(system, task);

        // Uppdatera den globala summan med denna tasks sum
        pthread_mutex_lock(system->lock);
        *system->sum += task->sum;
        pthread_mutex_unlock(system->lock);

        // Skriv ut lokal sum för tasken
        printf("%s\t%ld\n", task->path, task->sum);

        // Frigör task-minne
        free(task->path);
        free(task);
    }
}

static bool inode_seen(System *sys, dev_t st_dev, ino_t st_ino)
{
    pthread_mutex_lock(&sys->inode_lock);

    Inode *current = sys->seen_inodes;
    while(current != NULL) {
        if(current->st_dev == st_dev && current->st_ino == st_ino) {
            pthread_mutex_unlock(&sys->inode_lock);
            return true;  // redan räknad
        }
        current = current->next;
    }

    // Lägg till ny inode
    Inode *node = malloc(sizeof(Inode));
    node->st_dev = st_dev;
    node->st_ino = st_ino;
    node->next = sys->seen_inodes;
    sys->seen_inodes = node;

    pthread_mutex_unlock(&sys->inode_lock);
    return false;
}

