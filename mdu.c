/**
 * main.c - Multithreaded file block counting program.
 *
 * This program counts blocks in files using multiple threads.
 * It parses command-line arguments, initializes the system,
 * enqueues tasks for threads, waits for completion, prints results,
 * and cleans up allocated memory.
 * 
 * Author: Rasmus Mikaelsson (et24rmn)
 * version: 13-11-2025
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <linux/limits.h>
#include "system.h"

/* ------------------ Declarations of internal functions ------------------ */

static int parse_commandline(int argc, char **argv);

/* -------------------------- External functions -------------------------- */

int main(int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr, "Usage: %s [-j n_threads] file ...\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int n_threads = parse_commandline(argc, argv);
    if (n_threads < 0) {
        exit(EXIT_FAILURE);
	}

    pthread_t threads[n_threads];
    System system;

    /* Initialize system and threads */
    if (system_init(&system, threads, n_threads) < 0) {
        fprintf(stderr, "Initialization failed\n");
        exit(EXIT_FAILURE);
    }

	/* Initialize sums for each file */
	int file_count = argc - optind;
	blkcnt_t sums[file_count];
	for(int i = 0; i < file_count; i++) {
		sums[i] = 0;
	}

	/* Enqueue all start paths */
	for (int i = optind; i < argc; i++) {

		/* Create the new task */
		Task *task = malloc(sizeof(Task));
		if (!task) {
			perror("malloc Task");
			system_destroy(&system);
			exit(EXIT_FAILURE);
		}

		/* Create a valid path */
		task->path = malloc(PATH_MAX);
		if (!task->path) {
			perror("malloc path");
			free(task);
			system_destroy(&system );
			exit(EXIT_FAILURE);
		}

		snprintf(task->path, PATH_MAX, "%s", argv[i]);
		task->sum = &sums[i - optind];

		if (system_enqueue(&system, task) != 0) {
			free(task->path);
			free(task);
			system_destroy(&system);
			exit(EXIT_FAILURE);
		}
	}

    /* Wait for threads */
    system_join(&system, threads, n_threads);

    /* Print total :) */
	for(int i = 0; i < file_count; i++) {
		printf("%-8ld %s\n", sums[i], argv[i + optind]);
	}

    /* Free memory */
    system_destroy(&system);

    return 0;
}

/* -------------------------- Internal functions -------------------------- */

/**
 * parse_commandline - Parses -j flag and returns number of threads.
 * @argc: Argument count.
 * @argv: Argument vector.
 *
 * Return: Number of threads (>=1), or -1 on error.
 */
static int parse_commandline(int argc, char **argv)
{
    int opt;
    int n_threads = 1;

    while ((opt = getopt(argc, argv, "j:")) != -1) {
        switch (opt) {
        case 'j':
            if (atoi(optarg) > 0)
                n_threads = atoi(optarg);
            break;
        default:
            fprintf(stderr, "Usage: %s [-j n_threads] file ...\n", argv[0]);
            return -1;
        }
    }

    return n_threads;
}
