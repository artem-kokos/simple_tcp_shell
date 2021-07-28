#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>

#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/wait.h>

#define DEFAULT_SHELL "/bin/sh"


static void sigchld_handler(int signal) {
    int saved_errno;

    saved_errno = errno;

    while (waitpid(-1, NULL, WNOHANG) > 0)
        continue;

    errno = saved_errno;
}


int main(int argc, char** argv) {
    /* TODO: Parse cmdline */
    /* TODO: Daemonize it */
    int server_socket_fd;
    struct sockaddr_in server_addr;

    char* cmd_shell = NULL;

    struct sigaction sa;

    /* Prepare SIGCHLD handler */
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sa.sa_handler = sigchld_handler;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        fprintf(stderr, "ERROR: Can't init handler for SIGCHLD (%s)\n", strerror(errno));
        return -1;
    }

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

    cmd_shell = getenv("SHELL");
    if (!cmd_shell)
        cmd_shell = DEFAULT_SHELL;

    /* TODO: Make it interruptible, don't use while(1) */
    while (1) {
        /* TODO: Save client IP address here to display it later */
        int client_socket_fd;
        struct sockaddr_in client_addr;
        socklen_t client_addr_size = sizeof(struct sockaddr_in);

        client_socket_fd = accept(server_socket_fd, (struct sockaddr*)&client_addr, &client_addr_size);
        if (-1 == client_socket_fd) {
            switch (errno) {
                case EINTR:
                    /* Can occur if client disconnected while we are
                     * waiting another clients in accept()
                     */
                    break;

                default:
                    fprintf(stderr, "WARNING: Can't accept new connection (%s)\n", strerror(errno));
                    break;
            }

            continue;
        }

        pid_t pid = fork();
        if (0 == pid) {
            dup2(client_socket_fd, STDIN_FILENO);
            dup2(client_socket_fd, STDOUT_FILENO);
            dup2(client_socket_fd, STDERR_FILENO);

            execl(cmd_shell, cmd_shell, NULL);
        } else if (-1 == pid) {
            fprintf(stderr, "WARNING: Can't fork to new terminal (%s)\n", strerror(errno));
        } else {
            fprintf(stdout, "New connection from %s, process PID: %d\n", inet_ntoa(client_addr.sin_addr), pid);
        }
    }
    /* TODO: Close all on exit */

    close(server_socket_fd);
    return 0;
}
