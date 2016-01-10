#define SIMPSH_RDONLY 0
#define SIMPSH_WRONLY 1

const struct option options[] = {
	{ "rdonly", required_argument, NULL, SIMPSH_RDONLY },
	{ "wronly", required_argument, NULL, SIMPSH_WRONLY },
	{ 0, 0, 0, 0 }
};
