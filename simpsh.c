#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include "simpsh.h"

typedef struct command {
	// Array of argv elements.
	char** cmdstr;
	// Number of elements, including the command itself.
	int length;

	pid_t pid;
} command;

int max (int x, int y) {
	return (x > y) ? x : y;
}

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

int add_child(char **cmdstr, int length, pid_t pid,
	command **p, int *psize, int *pcapacity) {
	if (*psize == *pcapacity) {
		*pcapacity *= 2;
		command *newp = realloc(*p, sizeof(command) * *pcapacity);
		if (newp == NULL) {
			fprintf(stderr, "Error calling realloc.\n");
			return 1;
		}
		*p = newp;
	}
	(*p)[*psize] = (command) { cmdstr, length, pid };
	(*psize)++;
	return 0;
}

int close_fds(int *fd, int fd_size) {
	int ret = 0;
	for (int i = 0; i < fd_size; i++) {
		if (fd[i] >= 0 && close(fd[i]) == -1) {
			fprintf(stderr, "Error closing file #%d.\n", i);
			ret = 1;
		}
	}
	return ret;
}

int is_option_argument(char* arg) {
	// If arg is null, then we've reached the end of the argv array.
	// Otherwise make sure arg is empty or does not start with "--".
	return arg != NULL && (arg[0] == '\0' || !(arg[0] == '-' && arg[1] == '-'));
}

void catch_handler(int signal) {
	// Technically shouldn't use fprintf, so we'll manually print.
	char message[18 + 1] = "0123456789 caught\n";
	// Replace with actual signal number.
	int index = 9;
	int temp = signal;
	do {
		message[index] = '0' + (temp % 10);
		index--;
		temp /= 10;
	} while (temp != 0);
	write(STDERR_FILENO, message + index + 1, 18 - index);
	// Regular exit() is not safe in handlers.
	_exit(signal);
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

	int child_capacity = 8;
	command *child = malloc(sizeof(command) * child_capacity);
	if (child == NULL) {
		fprintf(stderr, "Error calling malloc.\n");
		return 1;
	}
	int child_size = 0;

	// Exit code.
	int status = 0;
	// Check for Verbose.
	int is_verbose = 0;
	// Flags for next file to be opened.
	int oflag = 0;
	// Process option by option.
	int opt, options_index;
	// Check for Ignore SIGSEGV.
	int ignore_abort = 0;
	while ((opt = getopt_long(argc, argv, "", options, &options_index)) != -1) {
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

		if (opt != '?' && opt != ':' && (
			(options[options_index].has_arg == no_argument && nargs > 0)
			|| (options[options_index].has_arg == required_argument && opt != SIMPSH_COMMAND && nargs > 1))) {
			// If required and no argument is provided, handle in switch.
			fprintf(stderr, "Ignoring extra argument(s) for %s.\n", argv[base_index]);
		}

		switch (opt) {
			// If getopt_long finds a missing argument (for instance) at the last option, just use its error message.
			case '?': case ':':
			{
				status = max(status, 1);
				break;
			}
			case SIMPSH_APPEND: oflag |= O_APPEND; break;
			case SIMPSH_CLOEXEC: oflag |= O_CLOEXEC; break;
			case SIMPSH_CREAT: oflag |= O_CREAT; break;
			case SIMPSH_DIRECTORY: oflag |= O_DIRECTORY; break;
			case SIMPSH_DSYNC: oflag |= O_DSYNC; break;
			case SIMPSH_EXCL: oflag |= O_EXCL; break;
			case SIMPSH_NOFOLLOW: oflag |= O_NOFOLLOW; break;
			case SIMPSH_NONBLOCK: oflag |= O_NONBLOCK; break;
			case SIMPSH_RSYNC: oflag |= O_RSYNC; break;
			case SIMPSH_SYNC: oflag |= O_SYNC; break;
			case SIMPSH_TRUNC: oflag |= O_TRUNC; break;
			case SIMPSH_RDONLY: case SIMPSH_WRONLY: case SIMPSH_RDWR:
			{
				int newfd;
				if (nargs == 0) {
					fprintf(stderr, "No argument provided for file #%d.\n", fd_size);
					newfd = -1;
				} else {
					if (opt == SIMPSH_RDONLY) oflag |= O_RDONLY;
					else if (opt == SIMPSH_WRONLY) oflag |= O_WRONLY;
					else oflag |= O_RDWR;
					// optarg is argument f indicating the file to open.
					newfd = open(optarg, oflag, file_permissions);
					if (newfd == -1) {
						fprintf(stderr, "Error opening file %s for file #%d.\n", optarg, fd_size);
						status = max(status, 1);
					}
				}
				// Per Piazza @48, we will still add -1 fd as a logical fd.
				status = max(status, add_fd(newfd, &fd, &fd_size, &fd_capacity));
				oflag = 0;
				break;
			}
			case SIMPSH_PIPE:
			{
				int newfd[2];
				if (pipe(newfd) == -1) {
					fprintf(stderr, "Error opening pipe.\n");
					status = max(status, 1);
					// Still add -1 fd's as logical fds.
					newfd[0] = newfd[1] = -1;
				}
				status = max(status, add_fd(newfd[0], &fd, &fd_size, &fd_capacity));
				status = max(status, add_fd(newfd[1], &fd, &fd_size, &fd_capacity));
				break;
			}
			case SIMPSH_COMMAND:
			{
				if (nargs < 4) {
					fprintf(stderr, "--command option requires at least 4 arguments, proceeding to next option.\n");
					status = max(status, 1);
					break;
				}

				char *cmd = argv[optind + 2];

				// Determine I/O files.
				int iofile[3];
				for (int i = 0; i < 3; i++) {
					// Parse each file number, converting to int.
					iofile[i] = strtol(argv[optind - 1 + i], NULL, 10);
					if (iofile[i] >= fd_size || iofile[i] < 0 || fd[iofile[i]] == -1) {
						fprintf(stderr, "Standard %s for command %s refers to undefined, closed, or problematic file; proceeding to next option.\n", ioname[i], cmd);
						status = max(status, 1);
						// To break out of the switch, since break will only break out of the loop.
						goto after_switch;
					}
				}
				
				// (nargs - 3 + 1) arguments since we don't include the 3 I/O arguments but add a NULL to the end.
				int cargs_size = nargs - 3 + 1;
				char **cargs = malloc(sizeof(char*) * cargs_size);
				if (cargs == NULL) {
					fprintf(stderr, "Error calling malloc for command %s, proceeding to next option.\n", cmd);
					status = max(status, 1);
					break;
				}
				// Copy arguments from argv, starting from the command itself, into cargs.
				memcpy(cargs, argv + optind + 2, sizeof(char*) * (cargs_size - 1));
				cargs[cargs_size - 1] = NULL;


				pid_t child_pid;
				// Fork a child process.
				if ((child_pid = fork()) < 0) {
					fprintf(stderr, "Error forking child process on command %s, proceeding to next option.\n", cmd);
					status = max(status, 1);
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

					// Close all file descriptors.
					// The standard I/O descriptors still exist, so their descriptions will not disappear.
					status = max(status, close_fds(fd, fd_size));

					// Execute command.
					if (execvp( cmd, cargs) == -1) {
						fprintf(stderr, "Error executing command %s, aborting command.\n", cmd);
						status = max(status, 1);
					}
				} else {
					// Pass pointer to the command/program in argv.
					status = max(status, add_child(
						argv + base_index + 4, nargs - 3, child_pid,
						&child, &child_size, &child_capacity
						));
				}

				free(cargs);
				break;
			}
			case SIMPSH_WAIT:
			{
				int child_status;
				pid_t pid;
				while((pid = wait(&child_status)) != -1) {
					if (WIFEXITED(child_status)) {
						printf("Exit status %d:", WEXITSTATUS(child_status));
						status = max(status, WEXITSTATUS(child_status));
					} else if (WIFSIGNALED(child_status)) {
						printf("Terminated by signal %d:", WTERMSIG(child_status));
						status = max(status, WTERMSIG(child_status));
					} else if (WIFSTOPPED(child_status)) {
						printf("Stopped by signal %d:", WSTOPSIG(child_status));
						status = max(status, WSTOPSIG(child_status));
					}
					int i = 0;
					while (i < child_size && child[i].pid != pid) {
						i++;
					}
					if (i == child_size) {
						printf(" unknown child process.\n");
					} else {
						for (int j = 0; j < child[i].length; j++) {
							printf(" %s", child[i].cmdstr[j]);
						}
						printf("\n");
						// In case this pid is reused.
						child[i].pid = -1;
					}
				}
				if (errno == EINTR) {
					fprintf(stderr, "Wait interrupted before all child processes exited.\n");
				}
				break;
			}
			case SIMPSH_CLOSE:
			{
				if (nargs == 0) {
					fprintf(stderr, "No argument provided for --close.\n");
					break;
				}

				int file = strtol(optarg, NULL, 10);
				if (file >= fd_size || file < 0 || fd[file] == -1) {
					fprintf(stderr, "File %d refers to undefined, closed, or problematic file; proceeding to next option.\n", file);
					status = max(status, 1);
				} else {
					if (close(fd[file]) == -1) {
						fprintf(stderr, "Error closing file %d.\n", file);
					}
					// Don't access this file again.
					fd[file] = -1;
				}
				break;
			}
		        case SIMPSH_VERBOSE:
			{
				is_verbose = 1;
				break;
			}
			case SIMPSH_ABORT:
			{
				if (ignore_abort == 1) {
					break;
				}
				int *a = NULL;
				int b = *a; // violating instruction
				fprintf(stderr, "simpsh should have aborted already!"); // testing
				break;
			}
			case SIMPSH_CATCH:
			{
				int signum = strtol(optarg, NULL, 10);
				struct sigaction act;
				act.sa_handler = &catch_handler;
				sigemptyset(&act.sa_mask);
				act.sa_flags = 0;
				if (sigaction(signum, &act, NULL) == -1) {
					if (errno == EINVAL) {
						fprintf(stderr, "Invalid signal number: %d\n", signum);
					}
					status = max(status, 1);
				}
				if (signum == SIGSEGV) {
					ignore_abort = 0;
				}
				break;
			}
			case SIMPSH_IGNORE:
			{
				int signum = strtol(optarg, NULL, 10);
				if (signal(signum, SIG_IGN) == SIG_ERR) {
					if (errno == EINVAL) {
						fprintf(stderr, "Invalid signal number: %d\n", signum);
					}
					status = max(status, 1);
				}
				if (signum == SIGSEGV) {
					ignore_abort = 1;
				}
				break;
			}
			case SIMPSH_DEFAULT:
			{
				int signum = strtol(optarg, NULL, 10);
				if (signal(signum, SIG_DFL) == SIG_ERR) {
					if (errno == EINVAL) {
						fprintf(stderr, "Invalid signal number: %d\n", signum);
					}
					status = max(status, 1);
				}
				if (signum == SIGSEGV) {
					ignore_abort = 0;
				}
				break;
			}
			case SIMPSH_PAUSE:
			{
				pause();
				break;
			}
		}
after_switch:
		// Advance optind to the next long option, silently ignoring extraneous short options/arguments.
		// Extra options/arguments should have been checked in the switch already.
		optind = base_index + nargs + 1;
	}

	status = max(status, close_fds(fd, fd_size));
	free(fd);

	return status;
}
