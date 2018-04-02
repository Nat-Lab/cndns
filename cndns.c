#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>
#include <poll.h>

int main(int argc, char **argv) {
    if (argc < 5) {
        fprintf(stderr, "Usage: %s <bind_addr> <bind_port> <remote_dns> <max_time>\n", argv[0]);
        exit(1);
    }

    int s_bind = socket(PF_INET, SOCK_DGRAM, IPPROTO_IP);
    int s_recv = socket(PF_INET, SOCK_DGRAM, IPPROTO_IP);

    struct sockaddr_in sin_local, sin_remote;

    sin_local.sin_family = AF_INET;
    sin_local.sin_addr.s_addr = inet_addr(argv[1]);
    sin_local.sin_port = htons(atoi(argv[2]));

    sin_remote.sin_family = AF_INET;
    sin_remote.sin_addr.s_addr = inet_addr(argv[3]);
    sin_remote.sin_port = htons(53);

    if(bind(s_bind, (struct sockaddr *)&sin_local, sizeof(sin_local)) < 0) {
        fprintf(stderr, "[ERR] Can't bind %s:%s: %s\n", argv[1], argv[2], strerror(errno));
        exit(1);
    }

    struct pollfd fds[1];
    memset(fds, 0 , sizeof(fds));
    fds[0].fd = s_recv;
    fds[0].events = POLLIN;
    fds[0].revents = 0;

    int max_time = atoi(argv[4]), n;

    socklen_t sz;
    char buf[65535];
    while(1) {
        sz = sizeof(sin_local);
        n = recvfrom(s_bind, buf, sizeof(buf), 0, (struct sockaddr *)&sin_local, &sz);
        if (n <= 0) continue;
        sendto(s_recv, buf, n, 0, (struct sockaddr *)&sin_remote, sizeof(sin_remote));
        sz = sizeof(sin_remote);
        while(poll(fds, 1, max_time))
            n = recvfrom(s_recv, buf, sizeof(buf), 0, (struct sockaddr *)&sin_remote, &sz);
        sendto(s_bind, buf, n, 0, (struct sockaddr *)&sin_local, sizeof(sin_local));
    }
}
