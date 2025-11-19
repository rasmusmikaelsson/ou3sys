#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <linux/limits.h>

#include "system.h"

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

int main(int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr, "Usage: %s [-j n_threads] file ...\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int n_threads = parse_commandline(argc, argv);
    if (n_threads < 0)
        exit(EXIT_FAILURE);

    pthread_t threads[n_threads];
    System system;

    /* Initialize system and threads */
    if (system_init(&system, threads, n_threads) < 0) {
        fprintf(stderr, "Initialization failed\n");
        exit(EXIT_FAILURE);
    }

    /* Enqueue all start paths */
    for (int i = optind; i < argc; i++) {
        char *path = malloc(PATH_MAX);
        memcpy(path, argv[i], strlen(argv[i]) + 1);

        system_enqueue(&system, path);
    }

    /* Wait for threads */
    system_join(&system, threads, n_threads);

    /* Print total */
    printf("Total size: %ld\n", *system.sum);

    /* Free memory */
    system_destroy(&system);

    return 0;
}
