/**
 * system.c - System management for multithreaded file block counting.
 *
 * Provides functions to initialize, enqueue tasks, join worker threads,
 * and destroy system resources in a thread-safe manner.
 * 
 * Author: Rasmus Mikaelsson (et24rmn)
 * Version: 13-11-2025
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <linux/limits.h>

#include "system.h"
#include "queue.h"
#include "worker.h"

/* ------------------ Declarations of internal functions ------------------ */

static int unlock_mutex(pthread_mutex_t *m);
static int lock_mutex(pthread_mutex_t *m);
static int signal_cond(pthread_cond_t *cond);
static int broadcast_cond(pthread_cond_t *cond);
static int join_thread(pthread_t thread);
static int init_cond(pthread_cond_t *cond);
static int init_mutex(pthread_mutex_t *lock);
static int destroy_cond(pthread_cond_t *cond);
static int destroy_mutex(pthread_mutex_t *lock);

/* -------------------------- External functions -------------------------- */

int system_enqueue(System *system, Task *task)
{   
    /* Lock mutex */
	if(lock_mutex(system->lock) != 0) {
		return -1;
	}

	if(enqueue(system->queue, task) != 0) {
		if(unlock_mutex(system->lock) != 0) {
			return -1;
		}
		return -1;
	}
    
	if(signal_cond(system->cond) != 0) {
		if(unlock_mutex(system->lock) != 0) {
		    return -1;
		}
		return -1;
	}

	// Unlock mutex
    if(unlock_mutex(system->lock) != 0) {
		return -1;
	}

	// Return success
	return 0;
}

int system_join(System *system, pthread_t *threads, int n_threads)
{
	if(lock_mutex(system->lock) != 0) {
		return -1;
	}

	/* System is now done */
    *(system->done) = 1;

	if(broadcast_cond(system->cond) != 0) {
		return -1;
	}

	if(unlock_mutex(system->lock) != 0) {
		return -1;
	}

	/* Join threads */
    for (int i = 0; i < n_threads; i++) {
        if(join_thread(threads[i]) != 0) {
			return -1;
		}
    }
	
	/* Return success */
    return 0;
}

int system_init(System *system, pthread_t *threads, int n_threads)
{
	/* If any malloc failes, free successes and return -1 */ 
    pthread_cond_t *cond = malloc(sizeof(pthread_cond_t));
	if(!cond) {
		perror("malloc");
		return -1;
	}

    pthread_mutex_t *lock = malloc(sizeof(pthread_mutex_t));
	if(!lock) {
		perror("malloc");
        free(cond);
		return -1;
	}

    int *done = malloc(sizeof(int));
	if(!done) {
		perror("malloc");
		free(cond);
		free(lock);
		return -1;
	}

    blkcnt_t *sum = malloc(sizeof(blkcnt_t));
	if(!sum) {
		perror("malloc");
		free(cond);
		free(lock);
		free(done);
		return -1;
	}

	/* Initialization of condition varible and mutex lock */
	if(init_cond(cond) != 0) {
        free(cond);
		free(lock);
		free(done);
        free(sum);
		return -1;
	}

	if(init_mutex(lock) != 0) {
        free(cond);
		free(lock);
		free(done);
        free(sum);
		return -1;
	}

    *done = 0;
    *sum = 0;

    system->cond = cond;
    system->lock = lock;
    system->done = done;
    system->queue = create_queue();
    if(!system->queue) {
        free(cond);
		free(lock);
		free(done);
        free(sum);
        return -1;
    }
    system->sum = sum;

	if(init_mutex(&system->inode_lock) != 0) {
        free(cond);
		free(lock);
		free(done);
        free(sum);
		return -1;
	}

    system->seen_inodes = NULL;

    /* Create worker threads */
    for (int i = 0; i < n_threads; i++) {
        if (pthread_create(&threads[i], NULL, worker, system) != 0) {
            fprintf(stderr, "pthread creation failed\n");
            return -1;
        }
    }

	/* Return success */
    return 0;
}

int system_destroy(System *system)
{
	if(lock_mutex(&system->inode_lock) != 0) {
		return -1;
	}

    /* Free linked list of seen inodes */
    Inode *current = system->seen_inodes;
    while (current) {
        Inode *tmp = current;
        current = current->next;
        free(tmp);
    }

    system->seen_inodes = NULL;

	if(unlock_mutex(&system->inode_lock) != 0) {
		return -1;
	}

    free_queue(system->queue);

    /* Destroy mutexes and condition variable */
	if(destroy_cond(system->cond) != 0) {
		return -1;
	}

	if(destroy_mutex(system->lock) != 0) {
		return -1;
	}
   
    /* Free dynamically allocated memory */
    free(system->cond);
    free(system->lock);
    free(system->done);
    free(system->sum);

	/* Return success */
    return 0;
}

/* -------------------------- Internal functions -------------------------- */

/**
 * Unlocks the mutex
 *
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
 * 
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
 * Signals one thread waiting on a condition variable.
 *
 * @cond: Pointer to the condition variable to signal.
 *
 * Return: 0 on success, -1 on failure.
 */
static int signal_cond(pthread_cond_t *cond) {
    int ret = pthread_cond_signal(cond);
    if (ret != 0) {
        fprintf(stderr, "pthread_cond_signal failed: %s\n", strerror(ret));
        return -1;
    }
    return 0;
}

/**
 * broadcast_cond - Wakes up all threads waiting on a condition variable.
 *
 * @cond: Pointer to the condition variable to broadcast.
 *
 * Return: 0 on success, -1 on failure.
 */
static int broadcast_cond(pthread_cond_t *cond) {
    int ret = pthread_cond_broadcast(cond);
    if (ret != 0) {
        fprintf(stderr, "pthread_cond_broadcast failed: %s\n", strerror(ret));
        return -1;
    }
    return 0;
}

/**
 * Waits for a thread to finish execution.
 *
 * @thread: Thread identifier to join.
 *
 * Return: 0 on success, -1 on failure.
 */
static int join_thread(pthread_t thread) {
    int ret = pthread_join(thread, NULL);
    if (ret != 0) {
        fprintf(stderr, "pthread_join failed: %s\n", strerror(ret));
        return -1;
    }
    return 0;
}

/**
 * Initialize a condition variable with error handling.
 * @cond: Pointer to pthread condition variable.
 *
 * Return: 0 on success, -1 on failure.
 */
static int init_cond(pthread_cond_t *cond) {
    int ret = pthread_cond_init(cond, NULL);
    if (ret != 0) {
        fprintf(stderr, "pthread_cond_init failed: %s\n", strerror(ret));
        return -1;
    }
    return 0;
}

/**
 * Initialize a mutex with error handling.
 * @lock: Pointer to pthread mutex.
 *
 * Return: 0 on success, -1 on failure.
 */
static int init_mutex(pthread_mutex_t *lock) {
    int ret = pthread_mutex_init(lock, NULL);
    if (ret != 0) {
        fprintf(stderr, "pthread_mutex_init failed: %s\n", strerror(ret));
        return -1;
    }
    return 0;
}

/**
 * Destroys a condition variable with error handling.
 * @cond: Pointer to the condition variable to destroy.
 *
 * Return: 0 on success, -1 on failure.
 */
static int destroy_cond(pthread_cond_t *cond) {
    int ret = pthread_cond_destroy(cond);
    if (ret != 0) {
        fprintf(stderr, "pthread_cond_destroy failed: %s\n", strerror(ret));
        return -1;
    }
    return 0;
}

/**
 * Destroys a mutex with error handling.
 * @lock: Pointer to the mutex to destroy.
 *
 * Return: 0 on success, -1 on failure.
 */
static int destroy_mutex(pthread_mutex_t *lock) {
    int ret = pthread_mutex_destroy(lock);
    if (ret != 0) {
        fprintf(stderr, "pthread_mutex_destroy failed: %s\n", strerror(ret));
        return -1;
    }
    return 0;
}

