#include "tcpcli.h"
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

enum TcpcliState {
    STAT_ERROR,     // error
    STAT_WAIT,      // waiting
    STAT_CONNECTING,// connecting
    STAT_CONNECTED, // connected
};

static wsocket connect_to(const char *addr, const char *service)
{
    wsocket sock = INVALID_WSOCKET;

    struct addrinfo hints = {0};
    hints.ai_family = PF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

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
        if (connect(sock, p->ai_addr, p->ai_addrlen) == WSOCKET_ERROR &&
            wsocket_errno != WSOCKET_EINPROGRESS) {
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

int tcpcli_init(struct tcpcli *tcp, float conn_timeout, float inact_timeout, float reconn_wait)
{
    tcp->socket = INVALID_WSOCKET;
    tcp->state = STAT_ERROR;
    tcp->activity = 0.0;
    tcp->addr[0] = '\0';
    tcp->serv[0] = '\0';
    tcp->connect_timeout = conn_timeout;
    tcp->inactive_timeout = inact_timeout;
    tcp->reconnect_wait = reconn_wait;
    return 0;
}

int tcpcli_open(struct tcpcli *tcp, const char *addr, int port)
{
    if (tcp->socket != INVALID_WSOCKET) {
        return -1;
    }
    char serv[32];
    serv[0] = '\0';
    snprintf(serv, sizeof(serv), "%d", port);
    wsocket sock = connect_to(addr, serv);
    if (sock == INVALID_WSOCKET) {
        return -1;
    }
    tcp->socket = sock;
    tcp->state = STAT_CONNECTING;
    tcp->activity = local_monotonic_clock();
    snprintf(tcp->addr, sizeof(tcp->addr), "%s", addr);
    snprintf(tcp->serv, sizeof(tcp->serv), "%s", serv);
    return 0;
}

int tcpcli_isconnected(struct tcpcli *tcp)
{
    if (tcp && tcp->state == STAT_CONNECTED) {
        return 1;
    }
    return 0;
}


static int tcpcli_wait(struct tcpcli *tcp)
{
    if (tcp->socket == INVALID_WSOCKET && tcp->state != STAT_WAIT) {
        return -1;
    }
    double now = local_monotonic_clock();
    if (tcp->state == STAT_WAIT) { // check if wait timeout
        if (tcp->reconnect_wait == 0 || (tcp->reconnect_wait > 0 && tcp->reconnect_wait <= (now - tcp->activity))) {
            wsocket sock = connect_to(tcp->addr, tcp->serv);
            if (sock == INVALID_WSOCKET) { // connect failed
                return -1;
            }
            tcp->socket = sock;
            tcp->state = STAT_CONNECTING;
            tcp->activity = now;
        }
    } else if (tcp->state == STAT_CONNECTING) { // check if connect or timeout
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(tcp->socket, &fds);
        struct timeval tv = {0};

        int err = select(tcp->socket + 1, NULL, &fds, NULL, &tv);
        if (err == 0) {
            // check if timeout
            if (tcp->connect_timeout > 0 && (now - tcp->activity >= tcp->connect_timeout)) { // timeout and reconnect
                tcp->state = STAT_ERROR;
            }
        } else if (err == -1) {
            // error
            tcp->state = STAT_ERROR;
        } else {
            // check connected
            int so_err = 0;
            socklen_t len = sizeof(so_err);
            if (getsockopt(tcp->socket, SOL_SOCKET, SO_ERROR, &so_err, &len) == -1 || so_err != 0) {
                // failed
                tcp->state = STAT_ERROR;
            } else {
                // OK
                tcp->state = STAT_CONNECTED;
                tcp->activity = now;
            }
        }
    } else if (tcp->state == STAT_CONNECTED) {
        // check if inactive
        if (tcp->inactive_timeout > 0 && (now - tcp->activity >= tcp->inactive_timeout)) { // timeout and reconnect
            tcp->state = STAT_ERROR;
        }
    }
    if (tcp->state == STAT_ERROR) { // change to wait
        if (tcp->reconnect_wait < 0) {
            // need wait forever, so report error
            wsocket_close(tcp->socket);
            tcp->socket = INVALID_WSOCKET;
            return -1;
        }
        wsocket_close(tcp->socket);
        tcp->socket = INVALID_WSOCKET;
        tcp->state = STAT_WAIT;
        tcp->activity = now;
    }
    return 0;
}

int tcpcli_read(struct tcpcli *tcp, void *buff, size_t count)
{
    if (tcpcli_wait(tcp) != 0) {
        return -1;
    }
    if (tcp->state == STAT_CONNECTED) {
        int rv = recv(tcp->socket, buff, count, 0);
        if ((rv == -1 && wsocket_errno != WSOCKET_EWOULDBLOCK) || rv == 0) {
            tcp->state = STAT_ERROR;
        }
        if (rv > 0) {
            tcp->activity = local_monotonic_clock();
            return rv;
        }
    }
    return 0;
}

int tcpcli_write(struct tcpcli *tcp, const void *data, size_t count)
{
    if (tcpcli_wait(tcp) != 0) {
        return -1;
    }
    if (tcp->state == STAT_CONNECTED) {
        int sd = send(tcp->socket, data, count, 0);
        if ((sd == -1 && wsocket_errno != WSOCKET_EWOULDBLOCK) || sd == 0) {
            tcp->state = STAT_ERROR;
        }
        if (sd > 0) {
            return sd;
        }
    }
    return 0;
}

double tcpcli_last_activity(struct tcpcli *tcp)
{
    if (tcp) {
        return local_monotonic_clock() - tcp->activity;
    } else {
        return -1;
    }
}

int tcpcli_close(struct tcpcli *tcp)
{
    if (tcp->socket != INVALID_WSOCKET) {
        wsocket_close(tcp->socket);
        tcp->socket = INVALID_WSOCKET;
        tcp->state = STAT_ERROR;
        tcp->activity = 0;
        tcp->addr[0] = '\0';
        tcp->serv[0] = '\0';
    }
    return 0;
}
