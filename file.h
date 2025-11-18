#ifndef FILE_H
#define FILE_H

#include "queue.h"

/**
 * Calculates the size of given file or directory.
 * @param file	File to get size of
 * @return		Size of file or directory, -1 on error
 */
int calculate_size(char *file, Queue *q);

#endif

