#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>


int main(int argc, char** argv) {
    /* TODO: Parse cmdline */
    /* TODO: Daemonize it */
    int server_socket_fd;
    struct sockaddr_in server_addr;

    memset(&server_addr, 0, sizeof(struct sockaddr_in));

    server_socket_fd = socket(AF_INET, SOCK_STREAM|SOCK_CLOEXEC, IPPROTO_TCP);
    if (-1 == server_socket_fd) {
        fprintf(stderr, "ERROR: Can't open socket (%s)\n", strerror(errno));
        return -1;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(4470); /* FIXME: Get port from cmdline */

    if (bind(server_socket_fd, (struct sockaddr*)&server_addr, sizeof(server_addr))) {
        fprintf(stderr, "ERROR: Can't bind address (%s)\n", strerror(errno));
        return -1;
    }

    if (listen(server_socket_fd, 1)) {
        fprintf(stderr, "ERROR: Can't start listening (%s)\n", strerror(errno));
        return -1;
    }

    /* TODO: Make it interruptible, don't use while(1) */
    while (1) {
        /* TODO: Save client IP address here to display it later */
        int client_socket_fd;
        struct sockaddr_in client_addr;
        socklen_t client_addr_size = sizeof(struct sockaddr_in);

        client_socket_fd = accept(server_socket_fd, (struct sockaddr*)&client_addr, &client_addr_size);
        if (-1 == client_socket_fd) {
            fprintf(stderr, "WARNING: Can't apply accept for new connection (%s)\n", strerror(errno));
            continue;
        }

        pid_t pid = fork();
        if (0 == pid) {
            /* Close listening socket in child just in case.
             * https://stackoverflow.com/a/32403609
             */
            close(server_socket_fd);

            dup2(client_socket_fd, STDIN_FILENO);
            dup2(client_socket_fd, STDOUT_FILENO);
            dup2(client_socket_fd, STDERR_FILENO);

            /* TODO: Make customizable, do not hard-code `bash' */
            execl("/bin/bash", "/bin/bash", NULL);
        } else if (-1 == pid) {
            close(client_socket_fd);
            fprintf(stderr, "WARNING: Can't fork to new terminal (%s)\n", strerror(errno));
            continue;
        } else {
            close(client_socket_fd);
            fprintf(stdout, "New connection from %s, process PID: %d\n", inet_ntoa(client_addr.sin_addr), pid);
        }

        /* TODO: waitpid() */
        /* TODO: Close all on exit and handle signals */
    }

    close(server_socket_fd);
    return 0;
}
