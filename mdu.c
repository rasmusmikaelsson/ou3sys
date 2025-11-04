#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "file.h"

/* ------------------ Declarations of internal functions ------------------ */

static int parse_commandline(int argc, char **argv, int num_of_threads);

/* -------------------------- External functions -------------------------- */

int main(int argc, char **argv) {
	int num_of_threads = 1;

	if(argc < 2) {
		fprintf(stderr, "Usage: %s [-j num_of_threads] file ...\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	if(parse_commandline(argc, argv, num_of_threads) != 0) {
		fprintf(stderr, "Parsing of commandline failed\n");
	}

	//TODO:	For each file, get the size of that file.
	//		If file is dir, calculate all files in that dir.
	//		If dir is found in dir, repeat.
	for(int i = optind; i < argc; i++) {
		printf("File %d: %s\n", i, argv[i]);
		if(calculate_size(argv[i]) == -1) {
			exit(EXIT_FAILURE);
		}
	}

	return 0;
}

/* -------------------------- Internal functions -------------------------- */

/**
 * Parses the commandline.
 *
 * @param argc				Number of arguments
 * @param argv				Argument vector
 * @param num_of_threads	Number of threads the program uses,
 *							defaults to 1
 * @return					0 on sucess, 1 on error
 */
static int parse_commandline(int argc, char **argv, int num_of_threads) {
	int opt;
	while((opt = getopt(argc, argv, "j:")) != -1) {
		switch (opt) {
			case 'j':
				if(atoi(optarg) > 1) {
					num_of_threads = atoi(optarg);
				}
				printf("j flag found. Num of threads: %d\n", num_of_threads);
				break;
			case '?':
				printf("Unknown flag..\n");
				break;
			default:
				printf("Error\n");
				return 1;
		}
	}

	return 0;
}



