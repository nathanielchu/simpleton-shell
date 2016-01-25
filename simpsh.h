#define SIMPSH_RDONLY 0
#define SIMPSH_WRONLY 1
#define SIMPSH_COMMAND 2
#define SIMPSH_VERBOSE 3
#define SIMPSH_APPEND 4
#define SIMPSH_CLOEXEC 5
#define SIMPSH_CREAT 6
#define SIMPSH_DIRECTORY 7
#define SIMPSH_DSYNC 8
#define SIMPSH_EXCL 9
#define SIMPSH_NOFOLLOW 10
#define SIMPSH_NONBLOCK 11
#define SIMPSH_RSYNC 12
#define SIMPSH_SYNC 13
#define SIMPSH_TRUNC 14
#define SIMPSH_RDWR 15
#define SIMPSH_PIPE 16
#define SIMPSH_WAIT 17
#define SIMPSH_CLOSE 18
#include <getopt.h>
#include <sys/stat.h>

const struct option options[] = {
	{ "rdonly", required_argument, NULL, SIMPSH_RDONLY },
	{ "wronly", required_argument, NULL, SIMPSH_WRONLY },
	{ "command", required_argument, NULL, SIMPSH_COMMAND },
	{ "verbose", no_argument, NULL, SIMPSH_VERBOSE },
	{ "append", no_argument, NULL, SIMPSH_APPEND },
	{ "cloexec", no_argument, NULL, SIMPSH_CLOEXEC },
	{ "creat", no_argument, NULL, SIMPSH_CREAT },
	{ "directory", no_argument, NULL, SIMPSH_DIRECTORY },
	{ "dsync", no_argument, NULL, SIMPSH_DSYNC },
	{ "excl", no_argument, NULL, SIMPSH_EXCL },
	{ "nofollow", no_argument, NULL, SIMPSH_NOFOLLOW },
	{ "nonblock", no_argument, NULL, SIMPSH_NONBLOCK },
	{ "rsync", no_argument, NULL, SIMPSH_RSYNC },
	{ "sync", no_argument, NULL, SIMPSH_SYNC },
	{ "trunc", no_argument, NULL, SIMPSH_TRUNC },
	{ "rdwr", required_argument, NULL, SIMPSH_RDWR },
	{ "pipe", required_argument, NULL, SIMPSH_PIPE },
	{ "wait", no_argument, NULL, SIMPSH_WAIT },
	{ "close", required_argument, NULL, SIMPSH_CLOSE },
	{ 0, 0, 0, 0 }
};

const char *ioname[] = { "input", "output", "error" };

const mode_t file_permissions = S_IRUSR | S_IWUSR;
