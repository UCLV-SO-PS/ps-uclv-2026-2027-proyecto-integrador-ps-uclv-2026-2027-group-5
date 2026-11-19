#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "monitor.h"

#define PORT 6666
#define BACKLOG 10
#define BUFSZ 1024

static void write_all(int fd, const char *buf, size_t len) {
    size_t off = 0;
    while (off < len) {
        ssize_t n = write(fd, buf + off, len - off);
        if (n < 0) {
            if (errno == EINTR) continue;
            break;
        }
        if (n == 0) break;
        off += (size_t)n;
    }
}

static int send_all(int fd, const char *buf, size_t len) {
    size_t off = 0;
    while (off < len) {
        ssize_t n = send(fd, buf + off, len - off, 0);
        if (n < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        if (n == 0) return -1;
        off += (size_t)n;
    }
    return 0;
}

static ssize_t read_request(int fd, char *buf, size_t size) {
    size_t total = 0;

    if (size == 0) return -1;

    while (total < size - 1) {
        ssize_t n = recv(fd, buf + total, size - 1 - total, 0);
        if (n < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        if (n == 0) break;

        total += (size_t)n;

        if (memchr(buf, '\n', total) != NULL) break;
        if (total >= 6) break;
    }

    buf[total] = '\0';
    return (ssize_t)total;
}

static void normalize_code(const char *in, char *out, size_t out_size) {
    size_t i, j = 0;
    if (out_size == 0) return;

    for (i = 0; in[i] != '\0' && j + 1 < out_size; i++) {
        if (!isspace((unsigned char)in[i])) {
            out[j++] = in[i];
        }
    }
    out[j] = '\0';
}

static void serve_client(int client_fd) {
    char request[64];
    char code[16];
    char response[BUFSZ];
    ssize_t n;

    n = read_request(client_fd, request, sizeof(request));
    if (n <= 0) return;

    normalize_code(request, code, sizeof(code));
    monitor_format(code, response, sizeof(response));
    send_all(client_fd, response, strlen(response));
}

static void child_handle_client(int client_fd, int pipe_write_fd,
                                const struct sockaddr_in *cliaddr) {
    char ip[INET_ADDRSTRLEN];

    if (inet_ntop(AF_INET, &cliaddr->sin_addr, ip, sizeof(ip)) == NULL) {
        strcpy(ip, "desconocida");
    }

    write_all(pipe_write_fd, ip, strlen(ip) + 1);
    close(pipe_write_fd);

    serve_client(client_fd);
    close(client_fd);
}

static void sigchld_handler(int signo) {
    int saved_errno = errno;
    (void)signo;

    while (waitpid(-1, NULL, WNOHANG) > 0) {
        /* cosechar hijos */
    }

    errno = saved_errno;
}

static void setup_server_signals(void) {
    struct sigaction sa;

    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = SIG_IGN;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);

    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(SIGCHLD, &sa, NULL);
}

static int create_listen_socket(void) {
    int listenfd;
    int opt = 1;
    struct sockaddr_in servaddr;

    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd < 0) {
        perror("socket");
        return -1;
    }

    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt");
        close(listenfd);
        return -1;
    }

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(PORT);

    if (bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("bind");
        close(listenfd);
        return -1;
    }

    if (listen(listenfd, BACKLOG) < 0) {
        perror("listen");
        close(listenfd);
        return -1;
    }

    return listenfd;
}

static int run_self_test(int listenfd) {
    pid_t pid;
    int connfd;
    int status;
    struct sockaddr_in cliaddr;
    socklen_t clilen = sizeof(cliaddr);
    char ip[INET_ADDRSTRLEN];

    printf("[test-run] Base de datos inicializada.\n");
    printf("[test-run] Socket escuchando en puerto %d.\n", PORT);

    pid = fork();
    if (pid < 0) {
        perror("fork test");
        return 1;
    }

    if (pid == 0) {
        int fd;
        struct sockaddr_in addr;
        char buf[BUFSZ];
        ssize_t n;
        const char *request = "S01E01";

        close(listenfd);

        fd = socket(AF_INET, SOCK_STREAM, 0);
        if (fd < 0) {
            perror("socket test");
            _exit(1);
        }

        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(PORT);

        if (inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr) != 1) {
            perror("inet_pton test");
            close(fd);
            _exit(1);
        }

        if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
            perror("connect test");
            close(fd);
            _exit(1);
        }

        if (send_all(fd, request, strlen(request)) < 0) {
            perror("send test");
            close(fd);
            _exit(1);
        }

        n = recv(fd, buf, sizeof(buf) - 1, 0);
        if (n <= 0) {
            perror("recv test");
            close(fd);
            _exit(1);
        }

        buf[n] = '\0';
        close(fd);

        if (strstr(buf, "The Hedge Knight") != NULL) {
            printf("[test-run] Respuesta recibida:\n%s", buf);
            _exit(0);
        }

        fprintf(stderr, "[test-run] Respuesta inesperada:\n%s", buf);
        _exit(1);
    }

    connfd = accept(listenfd, (struct sockaddr *)&cliaddr, &clilen);
    if (connfd < 0) {
        perror("accept test");
        waitpid(pid, NULL, 0);
        return 1;
    }

    if (inet_ntop(AF_INET, &cliaddr.sin_addr, ip, sizeof(ip)) == NULL) {
        strcpy(ip, "desconocida");
    }

    printf("[test-run] Cliente de prueba conectado desde %s.\n", ip);

    serve_client(connfd);
    close(connfd);

    if (waitpid(pid, &status, 0) < 0) {
        perror("waitpid test");
        return 1;
    }

    if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
        printf("[test-run] OK: el socket respondió correctamente.\n");
        return 0;
    }

    fprintf(stderr, "[test-run] FALLÓ: el cliente de prueba no recibió la respuesta esperada.\n");
    return 1;
}

int main(int argc, char **argv) {
    int listenfd;
    int test_run = 0;

    if (argc > 2) {
        fprintf(stderr, "Uso: %s [--test-run]\n", argv[0]);
        return 1;
    }

    if (argc == 2) {
        if (strcmp(argv[1], "--test-run") == 0) {
            test_run = 1;
        } else {
            fprintf(stderr, "Uso: %s [--test-run]\n", argv[0]);
            return 1;
        }
    }

    monitor_init();

    if (test_run) {
        signal(SIGINT, SIG_IGN);
        signal(SIGCHLD, SIG_DFL);
    } else {
        setup_server_signals();
    }

    listenfd = create_listen_socket();
    if (listenfd < 0) return 1;

    if (test_run) {
        int rc = run_self_test(listenfd);
        close(listenfd);
        return rc;
    }

    printf("Servidor concurrente escuchando en el puerto %d...\n", PORT);
    fflush(stdout);

    for (;;) {
        struct sockaddr_in cliaddr;
        socklen_t clilen = sizeof(cliaddr);
        int connfd;
        int pipefd[2];
        pid_t pid;

        connfd = accept(listenfd, (struct sockaddr *)&cliaddr, &clilen);
        if (connfd < 0) {
            if (errno == EINTR) continue;
            perror("accept");
            continue;
        }

        if (pipe(pipefd) < 0) {
            perror("pipe");
            close(connfd);
            continue;
        }

        pid = fork();
        if (pid < 0) {
            perror("fork");
            close(connfd);
            close(pipefd[0]);
            close(pipefd[1]);
            continue;
        }

        if (pid == 0) {
            close(listenfd);
            close(pipefd[0]);
            child_handle_client(connfd, pipefd[1], &cliaddr);
            _exit(0);
        } else {
            char ip[INET_ADDRSTRLEN] = "desconocida";
            ssize_t n;

            close(connfd);
            close(pipefd[1]);

            n = read(pipefd[0], ip, sizeof(ip) - 1);
            if (n > 0) {
                ip[n] = '\0';
            }
            close(pipefd[0]);

            printf("Hijo PID %d atendió al cliente con IP %s\n",
                   (int)pid, ip);
            fflush(stdout);
        }
    }

    close(listenfd);
    return 0;
}