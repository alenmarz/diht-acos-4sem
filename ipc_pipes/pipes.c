#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>

/*
 * Well named pipe file descriptors
 */
typedef struct { int read_stream, write_stream; } stream_t;

/*
 * Reports error acording to errno and exits
 */
void report_error(char *additional_info) {
    perror(additional_info);
    exit(EXIT_FAILURE);
}

/*
 * Creates a pipe and maps it's file descriptors to stream_t structure
 */
stream_t create_stream() {
    int tmp_stream[2];
    if (pipe(tmp_stream) == -1) report_error("can't create pipe");
    stream_t stream = { tmp_stream[0], tmp_stream[1] };

    return stream;
}

/*
 * Closes stream_t (a.k.a. pipe) file descriptors
 *
 * XXX: ignores EBADF since closing the descriptor which already closed is
 *      considered to be standard situation
 */
void close_stream(stream_t stream) {
    if (close(stream.read_stream) == -1 && (errno == EINTR || errno == EIO))
        report_error("can't close pipe");
    if (close(stream.write_stream) == -1 && (errno == EINTR || errno == EIO))
        report_error("can't close pipe");
}

/*
 * Closes first elements_count streams
 */
void close_streams(stream_t *streams, size_t elements_count) {
    while (elements_count--) close_stream(streams[elements_count]);
}

int main(int argc, char **argv) {
    /* Actually, we can kepp only two pipes at the moment instead of
      * keeping all the pipes which will be used during execution...
      * Nobody cares
      */
    stream_t *streams = (stream_t *)calloc(argc - 2, sizeof(stream_t));
    if (streams == NULL) report_error("can't create pipes");
    for(int i = 0; i < argc - 2; i++) streams[i] = create_stream();

    pid_t *pids = (pid_t *)calloc(argc - 2, sizeof(pid_t));

    for (int i = 1; i < argc; i++) {
        pid_t pid = fork();
        if (pid == -1) report_error("can't fork new process");
        if (pid == 0) {
            /* child part */
            if (i > 1) dup2(streams[i - 2].read_stream, STDIN_FILENO);
            if (i + 1 != argc) dup2(streams[i - 1].write_stream, STDOUT_FILENO);
            close_streams(streams, argc - 2);
            execlp(argv[i], argv[i], 0);
            exit(EXIT_FAILURE);
        }
        if (pid > 0) {
            /* parent part */
            if (i > 1) close_stream(streams[i - 2]);
            pids[i - 1] = pid;
        }
    }

    close_stream(streams[argc - 3]);
    free(streams);

    for (int i = 0; i < argc - 2; i++) {
        int child_exit_status;
        waitpid(pids[i], &child_exit_status, 0);
        if (!WIFEXITED(child_exit_status) || WEXITSTATUS(child_exit_status)) {
            fprintf(
                stderr,
                "%s unexpectedly terminated or did not start\n",
                argv[i + 1]
            );
            // exit(EXIT_FAILURE);
        }
    }

    return 0;
}
