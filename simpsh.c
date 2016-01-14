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
	// Array of file descriptors. -1 indicates a problem with that file.
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
		// optarg is null when getopt consumed no arguments.
		// Index of the current long option.
		int base_index = optind - (optarg == NULL ? 1 : 2);
		for (int i = base_index + 1; is_option_argument(argv[i]); i++) {
			nargs++;
		}
		// Check for Verbose for next option.
		if (is_verbose == 1) {
			printf("%s", argv[base_index]);
			// Arguments.
			for (int i = 1; i <= nargs; i++) {
				printf(" %s", argv[base_index + i]);
			}
			printf("\n");
			fflush(stdout);
		}

		switch (opt) {
			// If getopt_long finds a missing argument (for instance) at the last option, just use its error message.
			case '?': case ':':
			{
				if (status < 1) {
					status = 1;
				}
				break;
			}
			case SIMPSH_RDONLY: case SIMPSH_WRONLY:
			{
				int newfd;
				if (nargs == 0) {
					fprintf(stderr, "No argument provided for file #%d.\n", fd_size);
					newfd = -1;
				} else {
					int oflag = (opt == SIMPSH_RDONLY) ? O_RDONLY : O_WRONLY;
					// optarg is argument f indicating the file to open.
					if (nargs > 1) {
						fprintf(stderr, "Too many arguments provided for file #%d, using first argument %s as file.\n", fd_size, optarg);
					}
					newfd = open(optarg, oflag);
					if (newfd == -1) {
						fprintf(stderr, "Error opening file %s for file #%d.\n", optarg, fd_size);
						if (status < 1) {
							status = 1;
						}
					}
				}
				// Per Piazza @48, we will still add -1 fd as a logical fd.
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

				// Determine I/O files.
				int iofile[3];
				for (int i = 0; i < 3; i++) {
					// Parse each file number, converting to int.
					iofile[i] = strtol(argv[optind - 1 + i], NULL, 10);
					if (iofile[i] >= fd_size || iofile[i] < 0 || fd[iofile[i]] == -1) {
						fprintf(stderr, "Standard %s for command %s refers to undefined file, or there was a problem opening it; proceeding to next option.\n", ioname[i], cmd);
						if (status < 1) {
							status = 1;
						}
						// To break out of the switch, since break will only break out of the loop.
						goto after_switch;
					}
				}
				
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
						if (dup2(fd[iofile[j]], j) == -1) {
							fprintf(stderr, "Error redirecting standard %s for command %s to file %d, aborting command.\n", ioname[j], cmd, iofile[j]);
							exit(1);
						}
					}

					// Execute command.
					if (execvp( cmd, cargs) == -1) {
						if (status < 1) {
							status = 1;
						}
					}
				}

				free(cargs);
				break;
			}
		        case SIMPSH_VERBOSE:
			{
				is_verbose = 1;
				if (nargs > 0) {
					fprintf(stderr, "Ignoring arguments to --verbose.\n");
				}
				break;
			}

		}
after_switch:
		// Advance optind to the next long option, silently ignoring extraneous short options/arguments.
		// Extra options/arguments should have been checked in the switch already.
		if (nargs > 1) {
			optind += nargs - 1;
		}
	}
		
	// Close files/free.
	for (int i = 0; i < fd_size; i++) {
		if (fd[i] >= 0 && close(fd[i]) == -1) {
			fprintf(stderr, "Error closing file.\n");
			if (status < 1) {
				status = 1;
			}
		}
	}
	free(fd);

	return status;
}
