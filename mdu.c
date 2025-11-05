#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include "file.h"

/* ------------------ Declarations of internal functions ------------------ */

static int parse_commandline(int argc, char **argv, int n_threads);
static void *thread_func(void *args);
static int join_threads(pthread_t *threads, int n_threads);

/* -------------------------- External functions -------------------------- */

int main(int argc, char **argv) {
	int n_threads = 0;
	int i, count;
	long int sum = 0;

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
	int counts[] = {1,2,3,4,5,6,7,8,9,10,11,12,13,14}; // DEBUG
	for(count = 0; count < n_threads; count++) {
		if(pthread_create(&threads[count], NULL, &thread_func, &counts[count]) != 0) {
			fprintf(stderr, "pthread creation failed\n");
			exit(EXIT_FAILURE);
		}
	}
	
	// For each file, calculate it's size
	for(i = optind; i < argc; i++) {
		printf("File %d: %s\n", i, argv[i]);
		long int size = calculate_size(argv[i]);
		if(size == -1) {
			exit(EXIT_FAILURE);
		}

		sum += size;
	}

	// join threads
	join_threads(threads, n_threads);

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
				printf("j flag found. Num of threads: %d\n", n_threads);
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

static void *thread_func(void *args) {
	int *t_count;
	t_count = (int *)args;
	printf("Hi, im thread %d\n", *t_count);
	return NULL;
}

static int join_threads(pthread_t *threads, int n_threads) {
	for(int i = 0; i < n_threads; i++) {
		pthread_join(threads[i], NULL);
	}

	return 0;
}

