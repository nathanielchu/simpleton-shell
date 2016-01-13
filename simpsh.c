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
		int *newp = realloc(*p, sizeof(int) * *pcapacity);
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

int is_option_argument(char* arg) {
	// If arg is null, then we've reached the end of the argv array.
	// Otherwise make sure arg is empty or does not start with "--".
	return arg != NULL && (arg[0] == '\0' || !(arg[0] == '-' && arg[1] == '-'));
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
		// Count number of arguments (anything not a long option) to this long option.
		int nargs = 0;
		// optarg is null when there were no arguments.
		if (optarg != NULL) {
			for (int i = optind - 1; is_option_argument(argv[i]); i++) {
				nargs++;
			}
		}
		// Check for Verbose for next option.
		if (is_verbose == 1) {
			// Index of the current long option.
			int index = optind - (nargs == 0 ? 1 : 2);
			printf("%s", argv[index]);
			// Arguments.
			for (int i = 1; i <= nargs; i++) {
				printf(" %s", argv[index + i]);
			}
			printf("\n");
			fflush(stdout);
		}

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
				if (add_fd(newfd, &fd, &fd_size, &fd_capacity) == 1 && status < 1) {
					status = 1;
				}
				break;
			}
			case SIMPSH_COMMAND:
			{
				if (nargs < 4) {
					fprintf(stderr, "--command option requires at least 4 arguments, proceeding to next option.\n");
					if (status < 1) {
						status = 1;
					}
					break;
				}
				
				char *cmd = argv[optind + 2];
				// (nargs - 3 + 1) arguments since we don't include the 3 I/O arguments but add a NULL to the end.
				int cargs_size = nargs - 3 + 1;
				char **cargs = malloc(sizeof(char*) * cargs_size);
				if (cargs == NULL) {
					fprintf(stderr, "Error calling malloc for command %s, proceeding to next option.\n", cmd);
					if (status < 1) {
						status = 1;
					}
					break;
				}
				// Copy arguments from argv, starting from the command itself, into cargs.
				memcpy(cargs, argv + optind + 2, sizeof(char*) * (cargs_size - 1));
				cargs[cargs_size - 1] = NULL;


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
					for (int j = 0; j < 3; j++) {
						// Parse each file number, converting to int.
						int fdnum = strtol(argv[optind - 1 + j], NULL, 10);

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
						if (fdnum >= fd_size || fdnum < 0) {
							fprintf(stderr, "Standard %s for command %s refers to invalid file, aborting command.\n", msg, cmd);
							exit(1);
						}
						if (dup2(fd[fdnum], j) == -1) {
							fprintf(stderr, "Error redirecting standard %s for command %s to file %d, aborting command.\n", msg, cmd, fdnum);
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
		// Advance optind to the next long option, silently ignoring extraneous short options/arguments.
		if (nargs > 1) {
			optind += nargs - 1;
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
