#include "udpcli.h"
#include <stdio.h>
#include <time.h>

static double local_timestamp(const struct timespec *ts)
{
    double stamp = ts->tv_sec;
    stamp += ts->tv_nsec * 1E-9;
    return stamp;
}

static double local_monotonic_clock()
{
    struct timespec ts = { 0 };
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return local_timestamp(&ts);
}

enum UdpcliState {
    STAT_ERROR,     // error
    STAT_WAIT,      // waiting
    STAT_CONNECTED, // connected
};

static wsocket connect_to(const char *addr, const char *service)
{
    wsocket sock = INVALID_WSOCKET;

    struct addrinfo hints = {0};
    hints.ai_family = PF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = IPPROTO_UDP;

    int rv = 0;
    struct addrinfo *ai = NULL;
    if ((rv = getaddrinfo(addr, service, &hints, &ai)) != 0) {
        return INVALID_WSOCKET;
    }
    for (const struct addrinfo *p = ai; p != NULL; p = p->ai_next) {
        sock = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (sock == INVALID_WSOCKET) {
            continue;
        }
        if (wsocket_set_nonblocking(sock) == WSOCKET_ERROR) {
            wsocket_close(sock);
            return INVALID_WSOCKET;
        }
        if (connect(sock, p->ai_addr, p->ai_addrlen) == WSOCKET_ERROR) {
            // connect error
            wsocket_close(sock);
            sock = INVALID_WSOCKET;
            continue;
        }
        // Got it!
        break;
    }

    freeaddrinfo(ai);
    ai = NULL;
    return sock;
}

int udpcli_init(struct udpcli *udp, float inact_timeout, float reconn_wait)
{
    udp->socket = INVALID_WSOCKET;
    udp->state = STAT_ERROR;
    udp->activity = 0.0;
    udp->addr[0] = '\0';
    udp->serv[0] = '\0';
    udp->inactive_timeout = inact_timeout;
    udp->reconnect_wait = reconn_wait;
    return 0;
}

int udpcli_open(struct udpcli *udp, const char *addr, int port)
{
    if (udp->socket != INVALID_WSOCKET) {
        return -1;
    }
    char serv[32];
    serv[0] = '\0';
    snprintf(serv, sizeof(serv), "%d", port);
    wsocket sock = connect_to(addr, serv);
    if (sock == INVALID_WSOCKET) {
        return -1;
    }
    udp->socket = sock;
    udp->state = STAT_CONNECTED;
    udp->activity = local_monotonic_clock();
    snprintf(udp->addr, sizeof(udp->addr), "%s", addr);
    snprintf(udp->serv, sizeof(udp->serv), "%s", serv);
    return 0;
}


static int udpcli_wait(struct udpcli *udp)
{
    if (udp->socket == INVALID_WSOCKET && udp->state != STAT_WAIT) {
        return -1;
    }
    double now = local_monotonic_clock();
    if (udp->state == STAT_WAIT) { // check if wait timeout
        if (udp->reconnect_wait == 0 || (udp->reconnect_wait > 0 && udp->reconnect_wait <= (now - udp->activity))) {
            wsocket sock = connect_to(udp->addr, udp->serv);
            if (sock == INVALID_WSOCKET) { // connect failed
                return -1;
            }
            udp->socket = sock;
            udp->state = STAT_CONNECTED;
            udp->activity = now;
        }
    } else if (udp->state == STAT_CONNECTED) {
        // check if inactive
        if (udp->inactive_timeout > 0 && (now - udp->activity >= udp->inactive_timeout)) { // timeout and reconnect
            udp->state = STAT_ERROR;
        }
    }
    if (udp->state == STAT_ERROR) { // change to wait
        if (udp->reconnect_wait < 0) {
            // need wait forever, so report error
            wsocket_close(udp->socket);
            udp->socket = INVALID_WSOCKET;
            return -1;
        }
        wsocket_close(udp->socket);
        udp->socket = INVALID_WSOCKET;
        udp->state = STAT_WAIT;
        udp->activity = now;
    }
    return 0;
}

int udpcli_read(struct udpcli *udp, void *buff, size_t count)
{
    if (udpcli_wait(udp) != 0) {
        return -1;
    }
    if (udp->state == STAT_CONNECTED) {
        int rv = recv(udp->socket, buff, count, 0);
        if ((rv == -1 && wsocket_errno != WSOCKET_EWOULDBLOCK) || rv == 0) {
            udp->state = STAT_ERROR;
        }
        if (rv > 0) {
            udp->activity = local_monotonic_clock();
            return rv;
        }
    }
    return 0;
}

int udpcli_write(struct udpcli *udp, const void *data, size_t count)
{
    if (udpcli_wait(udp) != 0) {
        return -1;
    }
    if (udp->state == STAT_CONNECTED) {
        int sd = send(udp->socket, data, count, 0);
        if ((sd == -1 && wsocket_errno != WSOCKET_EWOULDBLOCK) || sd == 0) {
            udp->state = STAT_ERROR;
        }
        if (sd > 0) {
            return sd;
        }
    }
    return 0;
}

double udpcli_last_activity(struct udpcli *udp)
{
    if (udp) {
        return local_monotonic_clock() - udp->activity;
    } else {
        return -1;
    }
}

int udpcli_close(struct udpcli *udp)
{
    if (udp->socket != INVALID_WSOCKET) {
        wsocket_close(udp->socket);
        udp->socket = INVALID_WSOCKET;
        udp->state = STAT_ERROR;
        udp->activity = 0;
        udp->addr[0] = '\0';
        udp->serv[0] = '\0';
    }
    return 0;
}
