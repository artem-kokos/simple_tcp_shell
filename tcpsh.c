#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <ctype.h>

#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/wait.h>

#define DEFAULT_SHELL "/bin/sh"
#define DEFAULT_PORT 4470


static void sigchld_handler(int signal) {
    int saved_errno;

    saved_errno = errno;

    while (waitpid(-1, NULL, WNOHANG) > 0)
        continue;

    errno = saved_errno;
}


static int get_port(const char* port_string) {
    /* atoi() not best approach, because the
     * string which starts with digit (`d') and ends
     * with character (`c') is valid for the function.
     * There is no issues, it works good
     * just not beautiful in case of `dddc' or
     * something like this.
     */
    int port = atoi(port_string);

    if (port < 1 || port > 65535)
        return -1;
    return port;
}


int main(int argc, char** argv) {
    int server_socket_fd;
    struct sockaddr_in server_addr;

    char* cmd_shell = NULL;
    int server_port = DEFAULT_PORT;

    struct sigaction sa;

    if (argc > 1) {
        server_port = get_port(argv[1]);

        if (server_port == -1) {
            fprintf(stderr, "ERROR: Wrong port specified\n");
            return -1;
        }
    }

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
    server_addr.sin_port = htons(server_port);
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

    while (1) {
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

    close(server_socket_fd);
    return 0;
}
