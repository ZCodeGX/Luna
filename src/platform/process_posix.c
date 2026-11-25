#define _POSIX_C_SOURCE 200809L

#include "platform_internal.h"

#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define LUNA_PROCESS_OUTPUT_LIMIT ((size_t)8 * 1024 * 1024)

typedef struct {
    char* data;
    size_t len;
    size_t cap;
} ProcBuf;

static void pb_init(ProcBuf* b) {
    b->data = NULL;
    b->len = 0;
    b->cap = 0;
}

static void pb_free(ProcBuf* b) {
    free(b->data);
    pb_init(b);
}

static int pb_append(ProcBuf* b, const char* data, size_t n) {
    if (b->len >= LUNA_PROCESS_OUTPUT_LIMIT) return 0;
    if (n > LUNA_PROCESS_OUTPUT_LIMIT - b->len) {
        n = LUNA_PROCESS_OUTPUT_LIMIT - b->len;
    }
    if (b->len + n + 1 > b->cap) {
        size_t cap = b->cap ? b->cap : 256;
        while (cap < b->len + n + 1) {
            if (cap > (size_t)-1 / 2) return -1;
            cap *= 2;
        }
        char* next = (char*)realloc(b->data, cap);
        if (!next) return -1;
        b->data = next;
        b->cap = cap;
    }
    memcpy(b->data + b->len, data, n);
    b->len += n;
    b->data[b->len] = '\0';
    return 0;
}

static char* pb_take(ProcBuf* b, size_t* len) {
    *len = b->len;
    char* data = b->data;
    b->data = NULL;
    b->len = 0;
    b->cap = 0;
    return data;
}

static void child_fail(const char* message) {
    size_t len = strlen(message);
    (void)write(STDERR_FILENO, message, len);
    _exit(127);
}

static void set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags >= 0) {
        (void)fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    }
}

int luna_platform_run_process(const char* command, const char* cwd, int capture,
                              char** out_stdout, size_t* out_stdout_len,
                              char** out_stderr, size_t* out_stderr_len,
                              int* out_code, char* error_out, size_t error_size) {
    if (out_stdout) {
        *out_stdout = NULL;
        if (out_stdout_len) *out_stdout_len = 0;
    }
    if (out_stderr) {
        *out_stderr = NULL;
        if (out_stderr_len) *out_stderr_len = 0;
    }
    if (out_code) *out_code = -1;

    if (!command) {
        platform_set_error(error_out, error_size, "comando vacío");
        return LUNA_PLATFORM_ERROR;
    }

    int out_pipe[2] = { -1, -1 };
    int err_pipe[2] = { -1, -1 };
    if (capture) {
        if (pipe(out_pipe) != 0) {
            platform_set_error(error_out, error_size, "pipe falló: %s", strerror(errno));
            return LUNA_PLATFORM_ERROR;
        }
        if (pipe(err_pipe) != 0) {
            platform_set_error(error_out, error_size, "pipe falló: %s", strerror(errno));
            close(out_pipe[0]);
            close(out_pipe[1]);
            return LUNA_PLATFORM_ERROR;
        }
    }

    signal(SIGPIPE, SIG_IGN);
    pid_t pid = fork();
    if (pid < 0) {
        platform_set_error(error_out, error_size, "fork falló: %s", strerror(errno));
        if (capture) {
            close(out_pipe[0]); close(out_pipe[1]);
            close(err_pipe[0]); close(err_pipe[1]);
        }
        return LUNA_PLATFORM_ERROR;
    }

    if (pid == 0) {
        if (capture) {
            if (dup2(out_pipe[1], STDOUT_FILENO) < 0 ||
                dup2(err_pipe[1], STDERR_FILENO) < 0) {
                child_fail("luna: no se pudo redirigir la salida\n");
            }
            close(out_pipe[0]); close(out_pipe[1]);
            close(err_pipe[0]); close(err_pipe[1]);
        }
        if (cwd && chdir(cwd) != 0) {
            child_fail("luna: no se pudo cambiar de directorio\n");
        }
        execl("/bin/sh", "sh", "-c", command, (char*)NULL);
        child_fail("luna: no se pudo iniciar el intérprete de comandos\n");
    }

    ProcBuf out;
    ProcBuf err;
    pb_init(&out);
    pb_init(&err);
    int failed = 0;

    if (capture) {
        close(out_pipe[1]);
        close(err_pipe[1]);
        set_nonblocking(out_pipe[0]);
        set_nonblocking(err_pipe[0]);

        int open_out = 1;
        int open_err = 1;
        while (open_out || open_err) {
            struct pollfd pfd[2];
            pfd[0].fd = open_out ? out_pipe[0] : -1;
            pfd[1].fd = open_err ? err_pipe[0] : -1;
            pfd[0].events = POLLIN;
            pfd[1].events = POLLIN;
            pfd[0].revents = 0;
            pfd[1].revents = 0;

            int ready = poll(pfd, 2, -1);
            if (ready < 0) {
                if (errno == EINTR) continue;
                platform_set_error(error_out, error_size, "poll falló: %s", strerror(errno));
                failed = 1;
                break;
            }

            for (int i = 0; i < 2; i++) {
                if (pfd[i].fd < 0 || pfd[i].revents == 0) continue;
                char chunk[8192];
                ssize_t n = read(pfd[i].fd, chunk, sizeof(chunk));
                if (n > 0) {
                    if (pb_append(i == 0 ? &out : &err, chunk, (size_t)n) != 0) {
                        failed = 1;
                    }
                } else if (n == 0) {
                    close(pfd[i].fd);
                    if (i == 0) open_out = 0;
                    else open_err = 0;
                } else if (errno != EINTR && errno != EAGAIN) {
                    platform_set_error(error_out, error_size, "read falló: %s", strerror(errno));
                    failed = 1;
                    close(pfd[i].fd);
                    if (i == 0) open_out = 0;
                    else open_err = 0;
                }
            }
        }
        if (open_out) close(out_pipe[0]);
        if (open_err) close(err_pipe[0]);
    }

    int status = 0;
    pid_t waited;
    do {
        waited = waitpid(pid, &status, 0);
    } while (waited < 0 && errno == EINTR);
    if (waited < 0) {
        platform_set_error(error_out, error_size, "waitpid falló: %s", strerror(errno));
        failed = 1;
    }

    if (failed) {
        pb_free(&out);
        pb_free(&err);
        return LUNA_PLATFORM_ERROR;
    }

    if (WIFEXITED(status)) *out_code = WEXITSTATUS(status);
    else if (WIFSIGNALED(status)) *out_code = 128 + WTERMSIG(status);
    else *out_code = -1;

    if (capture) {
        *out_stdout = pb_take(&out, out_stdout_len);
        *out_stderr = pb_take(&err, out_stderr_len);
    }
    return LUNA_PLATFORM_OK;
}
