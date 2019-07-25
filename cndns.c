#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <errno.h>
#include <string.h>
#include <poll.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#define VERSION "0.2.2"

void help(char *me) {
    fprintf(stderr, "cndns %s\n\n", VERSION);
    fprintf(stderr, "usage: %s\n", me);
    fprintf(stderr, "   -s <remote_dns>     remote DNS\n");
    fprintf(stderr, "   -m <min_time>       minimum respond time (ms)\n");
    fprintf(stderr, "   -M [max_time]       lookup timeout (ms) (default: min_time + 1000)\n");
    fprintf(stderr, "   -l [bind_addr]      local address (default: 0.0.0.0)\n");
    fprintf(stderr, "   -p [bind_port]      local port (default: 53)\n");
    fprintf(stderr, "   -S                  strict mode (do nothing when timed out)\n");
    fprintf(stderr, "   -v                  debug output\n\n");
    exit(EXIT_FAILURE);
}

int main(int argc, char **argv) {
    char *bindaddr = "0.0.0.0", *remoteaddr;
    bool remoteaddr_set = false, mintime_set = false, debug = false, strict = false;
    in_port_t bindport = 53;
    time_t min_time = 0, max_time = 0;
    int opt;

    while ((opt = getopt(argc, argv, "s:m:M:l:p:Sv")) != -1) {
        switch (opt) {
            case 's':
                remoteaddr_set = true;
                remoteaddr = strdup(optarg);
                break;
            case 'm':
                mintime_set = true;
                min_time = atoi(optarg);
                break;
            case 'M':
                max_time = atoi(optarg);
                break;
            case 'l':
		bindaddr = strdup(optarg);
                break;
            case 'p':
                bindport = atoi(optarg);
                break;
            case 'v':
                debug = true;
                break;
            case 'S':
                strict = true;
                break;
            default:
                help(argv[0]);
                break;
        }
    }

    if (!remoteaddr_set || !mintime_set) help(argv[0]);

    int s_bind = socket(PF_INET, SOCK_DGRAM, IPPROTO_IP);
    int s_recv = socket(PF_INET, SOCK_DGRAM, IPPROTO_IP);

    struct sockaddr_in sin_local, sin_remote;

    sin_local.sin_family = AF_INET;
    sin_local.sin_addr.s_addr = inet_addr(bindaddr);
    sin_local.sin_port = htons(bindport);

    sin_remote.sin_family = AF_INET;
    sin_remote.sin_addr.s_addr = inet_addr(remoteaddr);
    sin_remote.sin_port = htons(53);

    if(bind(s_bind, (struct sockaddr *)&sin_local, sizeof(sin_local)) < 0) {
        fprintf(stderr, "[ERR] Can't bind %s:%hu: %s\n", bindaddr, bindport, strerror(errno));
        exit(EXIT_FAILURE);
    }

    struct pollfd fds[1];
    memset(fds, 0 , sizeof(fds));
    fds[0].fd = s_recv;
    fds[0].events = POLLIN;
    fds[0].revents = 0;

    max_time = (max_time ? max_time : min_time + 1000);
    int n;

    fprintf(stderr, "[INFO] listening on %s:%hu, forwarding to %s, max_time %ldms, min_time %ldms.\n", bindaddr, bindport, remoteaddr, max_time, min_time);
    socklen_t sz;
    char buf[65535];
    static struct timeval start, end;
    time_t r_time = 0;
    int ret;

    while(1) {
        sz = sizeof(sin_local);
        n = recvfrom(s_bind, buf, sizeof(buf), 0, (struct sockaddr *)&sin_local, &sz);
        if (n <= 0) continue;
        sendto(s_recv, buf, n, 0, (struct sockaddr *)&sin_remote, sizeof(sin_remote));
        sz = sizeof(sin_remote);
        r_time = 0;
        // no clock_gettime(), since I want it to run on OSX.
        gettimeofday(&start, NULL);
        while(r_time < min_time && (ret = poll(fds, 1, max_time - r_time))) {
            n = recvfrom(s_recv, buf, sizeof(buf), 0, (struct sockaddr *)&sin_remote, &sz);
            gettimeofday(&end, NULL);
            r_time = 1000 * (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000;
            if (debug) fprintf(stderr, "[DEBUG] got a result in %ldms.\n", r_time);
        }
        if (debug) fprintf(stderr, "[DEBUG] final result obtained in %ldms, due to %s.\n", r_time, (!ret) ? "timeout" : "in range");
        if (strict && !ret) {
            if (debug) fprintf(stderr, "[DEBUG] strict mode, out-of-range final result discarded.\n");
            continue;
        }
        sendto(s_bind, buf, n, 0, (struct sockaddr *)&sin_local, sizeof(sin_local));
    }
}
