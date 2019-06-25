# Simpleton Shell

Simpsh SIMPleton SHell is a simple stripped down shell. Instead of a scripting language, developers invoke the simpsh command by passing it arguments telling it which files to access, which pipes to create, and which subcommands to invoke. It then creates or accesses all the files and creates all the pipes processes needed to run the subcommands, and reports the processe's exit status as they exit.

## Options

Here is a detailed list of the command-line options that simpsh should support. Each option should be executed in sequence, left to right.

First are the file flags. These flags affect the next file that is opened. They are ignored if no later file is opened. Each file flag corresponds to an oflag value of open; the corresponding oflag value is listed after the option.

```
--append
    O_APPEND
--cloexec
    O_CLOEXEC
--creat
    O_CREAT
--directory
    O_DIRECTORY
--dsync
    O_DSYNC
--excl
    O_EXCL
--nofollow
    O_NOFOLLOW
--nonblock
    O_NONBLOCK
--rsync
    O_RSYNC
--sync
    O_SYNC
--trunc
    O_TRUNC
```

Second are the file-opening options. These flags open files. Each file-opening option also corresponds to an oflag value, listed after the option. Each opened file is given a file number; file numbers start at 0 and increment after each file-opening option. Normally they increment by 1, but the --pipe option causes them to increment by 2.

```
--rdonly f
    O_RDONLY. Open the file f for reading only.
--rdwr f
    O_RDWR. Open the file f for reading and writing.
--wronly f
    O_WRONLY. Open the file f for writing only.
--pipe
    Open a pipe. Unlike the other file options, this option does not take an argument. Also, it consumes two file numbers, not just one.
```

Third is the subcommand options:

```
--command i o e cmd args
    Execute a command with standard input i, standard output o and standard error e; these values should correspond to earlier file or pipe options. The executable for the command is cmd and it has zero or more arguments args. None of the cmd and args operands begin with the two characters "--".
--wait
    Wait for all commands to finish. As each finishes, output its exit status, and a copy of the command (with spaces separating arguments) to standard output.
```

Finally, there are some miscellaneous options:

```
--close N
    Close the Nth file that was opened by a file-opening option. For a pipe, this closes just one end of the pipe. Once file N is closed, it is an error to access it, just as it is an error to access any file number that has never been opened. File numbers are not reused by later file-opening options.
--verbose
    Just before executing an option, output a line to standard output containing the option. If the option has operands, list them separated by spaces. Ensure that the line is actually output, and is not merely sitting in a buffer somewhere.
--profile
    Just after executing an option, output a line to standard output containing the resources used. Use getrusage and output a line containing as much useful information as you can glean from it.
--abort
    Crash the shell. The shell itself should immediately dump core, via a segmentation violation.
--catch N
    Catch signal N, where N is a decimal integer, with a handler that outputs the diagnostic N caught to stderr, and exits with status N. This exits the entire shell. N uses the same numbering as your system; for example, on GNU/Linux, a segmentation violation is signal 11.
--ignore N
    Ignore signal N.
--default N
    Use the default behavior for signal N.
--pause
    Pause, waiting for a signal to arrive.
```

When there is a syntax error in an option (e.g., a missing operand), or where a file cannot be opened, or where is some other error in a system call, simpsh should report a diagnostic to standard error and should continue to the next option. However, simpsh should ignore any write errors to standard error, so that it does not get into an infinite loop outputting write-error diagnostics.

When simpsh exits other than in response to a signal, it should exit with status equal to the maximum of all the exit statuses of all the subcommands that it ran and successfully waited for. However, if there are no such subcommands, or if the maximum is zero, simpsh should exit with status 0 if all options succeeded, and with status 1 one of them failed. For example, if a file could not be opened, simpsh must exit with nonzero status.
