#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <linux/limits.h>

#include "system.h"
#include "queue.h"
#include "worker.h"   // <-- worker() is declared here

/**
 * system_enqueue - Enqueue a task in a thread-safe way.
 * @system: Pointer to System struct.
 * @task: Task to enqueue.
 */
void system_enqueue(System *system, void *task)
{
    pthread_mutex_lock(system->lock);
    enqueue(system->queue, task);
    pthread_cond_signal(system->cond);
    pthread_mutex_unlock(system->lock);
}

/**
 * system_join - Mark threads as done and join them.
 * @system: Pointer to System struct.
 * @threads: Thread array.
 * @n_threads: Number of threads.
 */
int system_join(System *system, pthread_t *threads, int n_threads)
{
    pthread_mutex_lock(system->lock);
    *(system->done) = 1;
    pthread_cond_broadcast(system->cond);
    pthread_mutex_unlock(system->lock);

    for (int i = 0; i < n_threads; i++) {
        pthread_join(threads[i], NULL);
    }

    return 0;
}

/**
 * system_init - Initialize System struct and create worker threads.
 * @system: Pointer to System.
 * @threads: Pre-allocated array of pthread_t.
 * @n_threads: Number of threads.
 */
int system_init(System *system, pthread_t *threads, int n_threads)
{
    pthread_cond_t *cond = malloc(sizeof(pthread_cond_t));
    pthread_mutex_t *lock = malloc(sizeof(pthread_mutex_t));
    int *done = malloc(sizeof(int));
    blkcnt_t *sum = malloc(sizeof(blkcnt_t));

    if (!cond || !lock || !done || !sum)
        return -1;

    pthread_cond_init(cond, NULL);
    pthread_mutex_init(lock, NULL);

    *done = 0;
    *sum = 0;

    system->cond = cond;
    system->lock = lock;
    system->done = done;
    system->queue = create_queue();
    system->sum = sum;

    pthread_mutex_init(&system->inode_lock, NULL);
    system->seen_inodes = NULL;

    // Create threads
    for (int i = 0; i < n_threads; i++) {
        if (pthread_create(&threads[i], NULL, worker, system) != 0) {
            fprintf(stderr, "pthread creation failed\n");
            return -1;
        }
    }

    return 0;
}

/**
 * system_destroy - Frees dynamically allocated fields inside System.
 * @system: Pointer to System struct.
 */
int system_destroy(System *system)
{
    pthread_mutex_lock(&system->inode_lock);
    Inode *current = system->seen_inodes;
    while (current) {
        Inode *tmp = current;
        current = current->next;
        free(tmp);
    }

    system->seen_inodes = NULL;
    pthread_mutex_unlock(&system->inode_lock);
    free_queue(system->queue);

    pthread_cond_destroy(system->cond);
    pthread_mutex_destroy(system->lock);

    free(system->cond);
    free(system->lock);
    free(system->done);
    free(system->sum);

    return 0;
}
