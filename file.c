#include "file.h"
#include "queue.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <limits.h>

/* -------------------------- External functions -------------------------- */

int calculate_size(char *filename, Queue *q) {
	struct stat sb;

	if(lstat(filename, &sb) == -1) {
		perror("lstat");
		return -1;
	}


	long int sum = sb.st_blocks;

	// Check if its a directory
	if (S_ISDIR(sb.st_mode)) {

		enqueue(q, filename);
		
		DIR *current_dir = opendir(filename);
		if (current_dir == NULL) {
			perror("opendir");
			return -1;
		}

		// Threads work:
		struct dirent *entry;
		while ((entry = readdir(current_dir)) != NULL) {
			// Skip . and .. dir
			if(strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
				continue;
			}

			char next_path[PATH_MAX];
			snprintf(next_path, PATH_MAX,"%s/%s", filename, entry->d_name);
			
			sum += calculate_size(next_path, q);
		}

		closedir(current_dir);
	}

	return sum;
}

