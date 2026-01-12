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
 *          12-01-2026 (Current)
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <linux/limits.h>
#include "system.h"

/* ------------------ Declarations of internal functions ------------------ */

static int parse_commandline(int argc, char **argv);
static blkcnt_t *init_sums(int file_count);
static int process_files(System *system, char **argv, int argc, int optind, pthread_t *threads, int n_threads);
static int enqueue_tasks(System *system, char **argv, int argc, int optind, blkcnt_t *sums);

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

	if (process_files(&system, argv, argc, optind, threads, n_threads) != 0) {
        system_destroy(&system);
        exit(EXIT_FAILURE);
    }

    /* Free memory */
    system_destroy(&system);

    return system.status;

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

/**
 * init_sums - Allocates and initializes an array for block count results.
 * @file_count: Number of files to allocate storage for.
 *
 * Returns: Pointer to initialized block count array, or NULL on failure.
 */
static blkcnt_t *init_sums(int file_count) {
    blkcnt_t *sums = calloc(file_count, sizeof(blkcnt_t));
    if (!sums) {
        perror("calloc sums");
        return NULL;
    }
    return sums;
}

/**
 * enqueue_tasks - Creates and enqueues initial tasks for all input paths.
 * @system: Pointer to the system structure.
 * @argv: Command-line argument vector.
 * @argc: Argument count.
 * @optind: Index of first non-option argument.
 * @sums: Array for storing block count results.
 *
 * Returns: 0 on success, -1 on failure.
 */
static int enqueue_tasks(System *system, char **argv, int argc, int optind, blkcnt_t *sums)
{
    for (int i = optind; i < argc; i++) {
        Task *task = malloc(sizeof(Task));
        if (!task)
            return -1;

        task->path = malloc(PATH_MAX);
        if (!task->path) {
            free(task);
            return -1;
        }

        snprintf(task->path, PATH_MAX, "%s", argv[i]);
        task->sum = &sums[i - optind];

        if (system_enqueue(system, task) != 0) {
            free(task->path);
            free(task);
            return -1;
        }
    }
    return 0;
}

/**
 * process_files - Processes all input paths and prints block usage results.
 * @system: Pointer to the system structure.
 * @argv: Command-line argument vector.
 * @argc: Argument count.
 * @optind: Index of first non-option argument.
 * @threads: Array of worker thread identifiers.
 * @n_threads: Number of worker threads.
 *
 * Returns: 0 on success, -1 on failure.
 */
static int process_files(System *system, char **argv, int argc, int optind, pthread_t *threads, int n_threads) {
    int file_count = argc - optind;
    blkcnt_t *sums = init_sums(file_count);
    if (!sums) {
        return -1;
	}
	
    if (enqueue_tasks(system, argv, argc, optind, sums) != 0) {
        free(sums);
        return -1;
    }

    system_join(system, threads, n_threads);

    for (int i = 0; i < file_count; i++){
        printf("%-8ld %s\n", sums[i], argv[i + optind]);
	}
	
    free(sums);
    return 0;
}
