
#include "file.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <limits.h>

/* ------------------ Declarations of internal functions ------------------ */

static int is_dir(const char *path);

/* -------------------------- External functions -------------------------- */

int get_size_of_file(char *file) {
	char path_buf[PATH_MAX + 1];
	char *dir_path = realpath(file, path_buf);
	// Check if file is a file or dir
	if (is_dir(dir_path)) {
		if (dir_path == NULL) {
			perror("realpath");
			return -1;
		}

		printf("realpath: %s\n", dir_path);

		DIR *dir = opendir(dir_path);
		if (dir == NULL) {
			perror("opendir");
			return -1;
		}

		struct dirent *entry;
		while ((entry = readdir(dir)) != NULL) {
			printf("%s/%s\n", dir_path, entry->d_name);
		}

		closedir(dir);
	}

	return 0;
}

/* -------------------------- Internal functions -------------------------- */

static int is_dir(const char *path) {
    struct stat sb;
	printf("Trying to lstat: %s\n", path);
    if (lstat(path, &sb) == -1) {
        perror("lstat");
        return 0;
    }
    return S_ISDIR(sb.st_mode);
}
