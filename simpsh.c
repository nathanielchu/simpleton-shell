#include <fcntl.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

// Pass pointers to cargs array, size, and capacity.
int add_cargs(char *newcargs, char ***p, int *psize, int *pcapacity) {
	if (*psize == *pcapacity) {
		*pcapacity *= 2;
		char **newp = realloc(*p, *pcapacity);
		if (newp == NULL) {
			fprintf(stderr, "Error calling realloc, proceeding to next option.\n");
			return 1;
		}
		*p = newp; 
	}
	(*p)[*psize] = newcargs;
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
	// Check for Verbose.
	int is_verbose = 0;
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
			case SIMPSH_COMMAND:
			{
				char *cmd = argv[optind + 2];

				int cargs_capacity = 8;
				int cargs_size = 0;
				char **cargs = malloc(sizeof(char*) * cargs_capacity);
				// Whether or not parsing of cargs has been successful.
				// We have to go through the arguments regardless to advance optind to the next option.
				int cargs_failed = 0;
				if (cargs == NULL) {
					fprintf(stderr, "Error calling malloc for command %s, proceeding to next option.\n", cmd);
					cargs_failed = 1;
				}

				// Parsing command arguments.
				// Need to add the command itself as the first "argument".
				int index = optind + 2;
				while (index < argc && argv[index][0] != '\0' && !(argv[index][0] == '-' && argv[index][1] == '-')) {
					if (!cargs_failed) {
						int temp = add_cargs(argv[index], &cargs, &cargs_size, &cargs_capacity);
						if (temp == 1) {
							cargs_failed = 1;
						}
					}
					// Regardless of cargs_failed we should increment index until we find the next option.
					index++;
				}
				// Save index of the 3 I/O file "arguments".
				int io_index = optind - 1;
				// Advance optind to next long option.
				optind = index;
				// If there was a problem storing cargs, abort.
				if (cargs_failed) {
					if (status < 1) {
						status = 1;
					}
					free(cargs);
					break;
				}
				// execvp needs last element to be NULL.
				add_cargs(NULL, &cargs, &cargs_size, &cargs_capacity);


				pid_t child_pid;
				// Fork a child process.
				if ((child_pid = fork()) < 0) {
					fprintf(stderr, "Error forking child process on command %s, proceeding to next option.\n", cmd);
					if (status < 1) {
						status = 1;
					}
					free(cargs);
					break;
				}

				// Execute command in the child process.
				if(child_pid == 0) {
					// Set command's standard io.		  
					long *io = malloc(sizeof(long) * 3);
					for (int j = 0; j < 3; j++) {
						// Parse each file number.
						io[j] = strtol(argv[io_index + j], NULL, 10);

						char *msg;
						switch(j) {
							case 0:
								msg = "input";
								break;
							case 1:
								msg = "output";
								break;
							case 2:
								msg = "error";
								break;
						}  						
						if ( (io[j] >= fd_size) || (io[j] < 0) ) {
							fprintf(stderr, "Standard %s for command %s refers to invalid file, aborting command.\n", msg, cmd);
							exit(1);
						}
						if (dup2(fd[io[j]], j) == -1) {
							fprintf(stderr, "Error redirecting standard %s for command %s to file %d, aborting command.\n", msg, cmd, io[j]);
							exit(1);
						}
					}

					// Execute command.
					execvp( cmd, cargs);
				}

				free(cargs);
				break;
			}
		        case SIMPSH_VERBOSE:
			{
				is_verbose = 1;
				break;
			}

		}
		// Check for Verbose.
		if (is_verbose == 1) {
			int index = optind;
			for(;;) {
				if ((argv[index] == NULL) ||
					(argv[index] == 0) ||
					(argv[index][0] == '-' && argv[index][1] == '-')) {					
					break;
				}				
				else {
					index++;
				}
			}
			for(;;) {
				if ((argv[index] == NULL) || (argv[index] == 0)) {					
					break;
				}
				fprintf(stdout, "%s ", argv[index]);
				index++;
				if ((argv[index] == NULL) ||
					(argv[index] == 0) ||
					(argv[index][0] == '-' && argv[index][1] == '-')) {
					fprintf(stdout, "%c", '\n');
					break;
				}
				else {
					fprintf(stdout, "%c", ' ');
				}
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
