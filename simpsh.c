#include <fcntl.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "simpsh.h"

// Pass pointers to fd array, size, and capacity.
int add_fd(int newfd, int **p, int *psize, int *pcapacity) {
	if (*psize == *pcapacity) {
		*pcapacity *= 2;
		int *newp = realloc(*p, *pcapacity);
		if (newp == NULL) {
			fprintf(stderr, "Error calling realloc, proceeding to next option.\n");
			return 1;
		}
		*p = newp;
	}
	(*p)[*psize] = newfd;
	(*psize)++;
	return 0;
}

int main(int argc, char *argv[]) {
	// Capacity of fd array.
	int fd_capacity = 8;
	// Array of file descriptors.
	int *fd = malloc(sizeof(int) * fd_capacity);
	if (fd == NULL) {
		fprintf(stderr, "Error calling malloc.\n");
		return 1;
	}
	// Number of file descriptors.
	int fd_size = 0;

	// Exit code.
	int status = 0;
	// Process option by option.
	int opt;
	while ((opt = getopt_long(argc, argv, "", options, NULL)) != -1) {
		switch (opt) {
			case SIMPSH_RDONLY: case SIMPSH_WRONLY:
			{
				int oflag = (opt == SIMPSH_RDONLY) ? O_RDONLY : O_WRONLY;
				// optarg is argument f indicating the file to open.
				int newfd = open(optarg, oflag);
				if (newfd == -1) {
					fprintf(stderr, "Error opening file %s, proceeding to next option.\n", optarg);
					if (status < 1) {
						status = 1;
					}
					break;
				}
				int temp = add_fd(newfd, &fd, &fd_size, &fd_capacity);
				if (temp == 1 && status < 1) {
					status = 1;
				}
				break;
			}
		}
	}

	// Close files/free.
	for (int i = 0; i < fd_size; i++) {
		if (close(fd[i]) == -1) {
			fprintf(stderr, "Error closing file.\n");
			if (status < 1) {
				status = 1;
			}
		}
	}
	free(fd);

	return status;
}
