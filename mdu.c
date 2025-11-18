#include <linux/limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>
#include <dirent.h>
#include <limits.h>
#include <sys/stat.h>
#include <string.h>
#include "file.h"
#include "queue.h"

typedef struct System {
	pthread_cond_t *cond;
	pthread_mutex_t *lock;
	int *done;
	Queue *queue;
	blkcnt_t *sum;
} System;


/* ------------------ Declarations of internal functions ------------------ */

static int parse_commandline(int argc, char **argv, int n_threads);
static void *worker(void *args);
static int join_threads(System *system, pthread_t *threads, int n_threads);
static void enqueue_task(System *system, void *task);

/* -------------------------- External functions -------------------------- */

int main(int argc, char **argv) {
	int n_threads = 0;
	int i, count;
	blkcnt_t sum = 0;
	
	pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
	pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
	int done = 0;
	Queue *queue = create_queue();

	System system;
	system.cond = &cond;
	system.lock = &lock;
	system.done = &done;
	system.queue = queue;
	system.sum = &sum;

	if(argc < 2) {
		fprintf(stderr, "Usage: %s [-j n_threads] file ...\n", argv[0]);
		exit(EXIT_FAILURE);
	}
	
	// Parse command options and set number of threads
	n_threads = parse_commandline(argc, argv, n_threads);
	if(n_threads < 0) {
		fprintf(stderr, "Parsing of commandline failed\n");
		exit(EXIT_FAILURE);
	}

	// Create threads given by user
	pthread_t threads[n_threads];
	for(count = 0; count < n_threads; count++) {
		if(pthread_create(&threads[count], NULL, &worker, &system) != 0) {
			fprintf(stderr, "pthread creation failed\n");
			exit(EXIT_FAILURE);
		}
	}
	
	// For each file, calculate it's size
	for(i = optind; i < argc; i++) {
		char *path = malloc(sizeof(char) * PATH_MAX);
		memcpy(path, argv[i], sizeof(char) * strlen(argv[i]) + 1);
		printf("File %d: %s\n", i, path);
		enqueue_task(&system, path);
	}

	// join threads
	join_threads(&system, threads, n_threads);
	free_queue(system.queue);

	printf("Total size: %ld\n", sum);

	return 0;
}

/* -------------------------- Internal functions -------------------------- */

/**
 * Parses the commandline.
 *
 * @param argc			Number of arguments
 * @param argv			Argument vector
 * @param n_threads		Number of threads the program uses
 * @return				Number of threads, -1 on error
 */
static int parse_commandline(int argc, char **argv, int n_threads) {
	int opt;
	while((opt = getopt(argc, argv, "j:")) != -1) {
		switch (opt) {
			case 'j':
				if(atoi(optarg) > 1) {
					n_threads = atoi(optarg);
				}
				break;
			case '?':
				printf("Unknown flag..\n");
				break;
			default:
				printf("Error\n");
				return -1;
		}
	}

	return n_threads;
}

static void enqueue_task(System *system, void *task) {
	pthread_mutex_lock(system->lock);
	enqueue(system->queue, task);
	pthread_cond_signal(system->cond);
	pthread_mutex_unlock(system->lock);
}

static void *worker(void *args) {
	System *system = (System *)args;
	printf("done: %d\n", *system->done);

	while(1) {
		// acquire a lock
		pthread_mutex_lock(system->lock);
		while (is_empty(system->queue) && *(system->done) == 0) {

			// let's wait on condition variable cond
			printf("Waiting on condition variable cond\n");
			pthread_cond_wait(system->cond, system->lock);
		}
		if(*(system->done) == 1 && is_empty(system->queue)) {
			pthread_mutex_unlock(system->lock);
			return NULL;
		}
		
		char *task = dequeue(system->queue);
		pthread_mutex_unlock(system->lock);
		printf("Next task: %s\n", task);
		struct stat sb;

		if(lstat(task, &sb) == -1) {
			perror("lstat");
			return NULL;
		}

		pthread_mutex_lock(system->lock);
		*(system->sum) += sb.st_blocks;
		pthread_mutex_unlock(system->lock);

		// Check if its a directory
		if (S_ISDIR(sb.st_mode)) {
			DIR *current_dir = opendir(task);
			if (current_dir == NULL) {
				perror("opendir");
				return NULL;	// TODO: Fix from NULL to a int FOR ALL
			}

			// Threads work:
			struct dirent *entry;
			while ((entry = readdir(current_dir)) != NULL) {
				// Skip . and .. dir
				if(strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
					continue;
				}

				char *next_path = malloc(sizeof(char) * PATH_MAX);
				snprintf(next_path, PATH_MAX,"%s/%s", task, entry->d_name);
				
				enqueue_task(system, next_path);
			}

			closedir(current_dir);
		}
		free(task);
	}
    printf("Returning thread\n");

	return NULL;
}

static int join_threads(System *system, pthread_t *threads, int n_threads) {
	pthread_mutex_lock(system->lock);
	*(system->done) = 1;
	pthread_cond_broadcast(system->cond);
	pthread_mutex_unlock(system->lock);

	for(int i = 0; i < n_threads; i++) {
		pthread_join(threads[i], NULL);
	}

	return 0;
}

