#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <errno.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <syslog.h>

#define PORT "9000"
#define DATA_FILE "/var/tmp/aesdsocketdata"

static int server_fd = -1;
static int client_fd = -1;
static volatile sig_atomic_t exit_requested = 0;

/* -------------------------------------------
   Signal handler: triggers proper shutdown
------------------------------------------- */
static void handle_signal(int sig)
{
    (void)sig;
    exit_requested = 1;
}

/* ----------------------------------------------------
   Convert sockaddr to numeric IP string
---------------------------------------------------- */
static void get_ip_str(const struct sockaddr *sa, socklen_t salen,
                       char *out, size_t outlen)
{
    if (getnameinfo(sa, salen, out, outlen, NULL, 0, NI_NUMERICHOST) != 0) {
        strncpy(out, "unknown", outlen);
        out[outlen - 1] = '\0';
    }
}

/* -------------------------------------------
   Read until newline and accumulate into buffer
   Returns length or -1 on error
------------------------------------------- */
static ssize_t recv_packet(int fd, char **buf_out)
{
    size_t size = 128;
    size_t used = 0;
    char *buf = malloc(size);
    if (!buf) return -1;

    while (1) {
        if (used + 1 >= size) {
            size *= 2;
            char *tmp = realloc(buf, size);
            if (!tmp) {
                free(buf);
                return -1;
            }
            buf = tmp;
        }

        ssize_t n = recv(fd, buf + used, 1, 0);
        if (n <= 0) {
            free(buf);
            return -1;
        }

        used += n;
        if (buf[used - 1] == '\n')
            break;
    }

    *buf_out = buf;
    return used;
}

/* -------------------------------------------
   Append received packet to data log file
------------------------------------------- */
static bool append_to_file(const char *buf, size_t len)
{
    int fd = open(DATA_FILE, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd < 0) return false;

    ssize_t written = write(fd, buf, len);
    close(fd);

    return (written == (ssize_t)len);
}

/* -------------------------------------------
   Send entire data log file back to client
------------------------------------------- */
static bool send_recvd_data(int fd)
{
    int f = open(DATA_FILE, O_RDONLY);
    if (f < 0) return false;

    char buf[1024];
    ssize_t n;

    while ((n = read(f, buf, sizeof(buf))) > 0) {
        ssize_t sent = send(fd, buf, n, 0);
        if (sent != n) {
            close(f);
            return false;
        }
    }

    close(f);
    return true;
}

/* -------------------------------------------
   Daemonize the process
------------------------------------------- */
static void create_deamon(void)
{
    pid_t pid = fork();
    if (pid < 0) exit(EXIT_FAILURE);
    if (pid > 0) exit(EXIT_SUCCESS);

    if (setsid() < 0) exit(EXIT_FAILURE);

    pid = fork();
    if (pid < 0) exit(EXIT_FAILURE);
    if (pid > 0) { 
        printf("The process ID of Child is %d\n", pid);
        exit(EXIT_SUCCESS);
    }

    umask(0);
    if (chdir("/") != 0) {
        syslog(LOG_ERR, "daemon chdir failed: %s", strerror(errno));
    }

    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
}

/* -------------------------------------------
   Main server loop
------------------------------------------- */
int main(int argc, char *argv[])
{   
    printf("Starting Server application in ");

    bool run_as_daemon = false;
    if (argc == 2 && strcmp(argv[1], "-d") == 0) {
        run_as_daemon = true;
    }

    (run_as_daemon) ? printf("deamon mode\n") : printf("normal mode\n");

    openlog("aesdsocket", LOG_PID, LOG_USER);

    unlink(DATA_FILE);
    printf("Data file cleared on startup\n");

    struct sigaction sa = {0};
    sa.sa_handler = handle_signal;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    /* Bind and listen */
    struct addrinfo hints = {0};
    struct addrinfo *res;

    hints.ai_family = AF_INET;          // IPv4
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if (getaddrinfo(NULL, PORT, &hints, &res) != 0) {
        syslog(LOG_ERR, "getaddrinfo failed");
        exit(EXIT_FAILURE);
    }

    server_fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (server_fd < 0) {
        syslog(LOG_ERR, "socket failed");
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    if (bind(server_fd, res->ai_addr, res->ai_addrlen) < 0) {
        syslog(LOG_ERR, "bind failed");
        exit(EXIT_FAILURE);
    }

    freeaddrinfo(res);

    if (run_as_daemon) {
        create_deamon();
    }

    if (listen(server_fd, 10) < 0) {
        syslog(LOG_ERR, "listen failed");
        exit(EXIT_FAILURE);
    } else {
        syslog(LOG_INFO, "Listening");
        printf("Listening\n");
    }

    /* Main accept loop */
    while (!exit_requested) {
        struct sockaddr_storage client_addr;
        socklen_t addrlen = sizeof(client_addr);

        printf("Waiting for connection request\n");

        client_fd = accept(server_fd,
                           (struct sockaddr *)&client_addr,
                           &addrlen);

        if (client_fd < 0) {
            if (exit_requested) break;
            syslog(LOG_ERR, "accept failed");
            continue;
        }

        char ipstr[NI_MAXHOST];
        get_ip_str((struct sockaddr *)&client_addr, addrlen, ipstr, sizeof(ipstr));
        syslog(LOG_INFO, "Accepted connection from %s", ipstr);
        printf("Accepted connection from %s", ipstr);

        char *packet = NULL;
        ssize_t packet_len = recv_packet(client_fd, &packet);

        if (packet_len > 0)
            append_to_file(packet, packet_len);

        free(packet);

        send_recvd_data(client_fd);

        syslog(LOG_INFO, "Closed connection from %s", ipstr);

        close(client_fd);
        client_fd = -1;
    }

    syslog(LOG_INFO, "Caught signal, exiting");

    close(server_fd);
    unlink(DATA_FILE);
    closelog();

    return 0;
}