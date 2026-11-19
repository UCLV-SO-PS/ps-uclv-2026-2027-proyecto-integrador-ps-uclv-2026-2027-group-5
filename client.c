#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 6666
#define SERVER_IP "127.0.0.1"
#define BUFSZ 1024

static void trim_newline(char *s) {
    size_t len = strlen(s);
    while (len > 0 && (s[len - 1] == '\n' || s[len - 1] == '\r')) {
        s[--len] = '\0';
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

int main(int argc, char **argv) {
    int fd;
    char code[32];
    char buf[BUFSZ];
    struct sockaddr_in server_addr;
    ssize_t n;

    if (argc > 2) {
        fprintf(stderr, "Uso: %s [codigo]\n", argv[0]);
        return 1;
    }

    if (argc == 2) {
        snprintf(code, sizeof(code), "%s", argv[1]);
    } else {
        printf("Introduzca código de episodio (ej. S01E01): ");
        if (fgets(code, sizeof(code), stdin) == NULL) {
            return 1;
        }
        trim_newline(code);
    }

    if (strlen(code) != 6) {
        fprintf(stderr, "El código debe tener exactamente 6 caracteres (ej. S01E01).\n");
        return 1;
    }

    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        perror("socket");
        return 1;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) != 1) {
        fprintf(stderr, "Dirección IP del servidor inválida.\n");
        close(fd);
        return 1;
    }

    if (connect(fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect");
        close(fd);
        return 1;
    }

    if (send_all(fd, code, strlen(code)) < 0) {
        perror("send");
        close(fd);
        return 1;
    }

    while ((n = recv(fd, buf, sizeof(buf) - 1, 0)) > 0) {
        buf[n] = '\0';
        fputs(buf, stdout);
    }

    if (n < 0) {
        perror("recv");
    }

    close(fd);
    return 0;
}