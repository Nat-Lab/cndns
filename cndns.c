#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>
#include <poll.h>
#include <time.h>

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

    sin_local.sin_family = AF_INET;
    sin_remote.sin_addr.s_addr=inet_addr(argv[3]);
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

    int max_time = atoi(argv[4]);

    while(1) {
        char buf[65535];
        int sz = sizeof(sin_local);
        int n = recvfrom(s_bind, buf, sizeof(buf), 0, (struct sockaddr *)&sin_local, &sz);
        if (n <= 0) continue;
        sendto(s_recv, buf, n, 0, (struct sockaddr *)&sin_remote, sizeof(sin_remote));
        while(1) {
            if (poll(fds, 1, max_time) > 0) {
                sz = sizeof(sin_remote);
                n = recvfrom(s_recv, buf, sizeof(buf), 0, (struct sockaddr *)&sin_remote, &sz);
            } else break;
        }
        sendto(s_bind, buf, n, 0, (struct sockaddr *)&sin_local, sizeof(sin_local));
    }
}
