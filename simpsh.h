#define SIMPSH_RDONLY 0
#define SIMPSH_WRONLY 1
#define SIMPSH_COMMAND 2
#define SIMPSH_VERBOSE 3

const struct option options[] = {
	{ "rdonly", required_argument, NULL, SIMPSH_RDONLY },
	{ "wronly", required_argument, NULL, SIMPSH_WRONLY },
	{ "command", required_argument, NULL, SIMPSH_COMMAND },
	{ "verbose", no_argument, NULL, SIMPSH_VERBOSE },
	{ 0, 0, 0, 0 }
};
