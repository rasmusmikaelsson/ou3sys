
#include "file.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <limits.h>

/* ------------------ Declarations of internal functions ------------------ */

static int is_file(char *filename);

/* -------------------------- External functions -------------------------- */

int get_size_of_file(char *file) {
	char path_buf[PATH_MAX];
	DIR *dir;

	// Check if file is a file or dir
	if(is_file(file) != 0) {
		
		// Get the path to the given directory
		char *dir_path = realpath(file, path_buf);
		
		// TODO:	Open directory and loop 
		//			trough files in the given directory
		if((dir = opendir(dir_path)) == NULL) {
			fprintf(stderr, "Directory failed to open\n");
		}
	} else {
		
	} 
}

/* -------------------------- Internal functions -------------------------- */

/** 
 * Checks if the file given is a file or a directory
 *
 * @param filename	File to check
 * @return			0 if target is not a file, otherwise 1
 */
static int is_file(char *filename) {
	FILE *check_file = fopen(filename, "r");
	if(check_file) {
		fclose(check_file);
		return 1;
	}
	return 0;
}
