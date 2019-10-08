#include "tcpsvr.h"

#include <stdio.h>

static wsocket listen_on(const char *addr, const char* service)
{
    wsocket sock = INVALID_WSOCKET;

    struct addrinfo hints = {0};
    hints.ai_family = PF_UNSPEC;
    hints.ai_flags = AI_PASSIVE;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    int rv = 0;
    struct addrinfo *ai = NULL;
    if ((rv = getaddrinfo(addr, service, &hints, &ai)) != 0) {
        return INVALID_WSOCKET;
    }
    for (const struct addrinfo *p = ai; p != NULL; p = p->ai_next) {
        sock = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (wsocket_set_nonblocking(sock) == WSOCKET_ERROR) {
            wsocket_close(sock);
            return INVALID_WSOCKET;
        }
        if (sock == INVALID_WSOCKET) {
            continue;
        }
        // enable addr resuse
        setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (const char *)&(int){1}, sizeof(int));
        if (bind(sock, p->ai_addr, p->ai_addrlen) == WSOCKET_ERROR) {
            // bind error
            wsocket_close(sock);
            sock = INVALID_WSOCKET;
            continue;
        }
        // Got it!
        break;
    }

    if (sock == INVALID_WSOCKET) {
        freeaddrinfo(ai);
        ai = NULL;
        return INVALID_WSOCKET;
    }

    freeaddrinfo(ai);
    ai = NULL;

    if (listen(sock, 2) == WSOCKET_ERROR) {
        wsocket_close(sock);
        return INVALID_WSOCKET;
    }
    return sock;
}

int tcpsvr_init(struct tcpsvr *svr, int readflag)
{
    svr->socket = INVALID_WSOCKET;
    for (int i = 0; i < TCPSVR_MAX_CLI; i++) {
        svr->clients[i] = INVALID_WSOCKET;
    }
    if (readflag >= TCPSVR_READ_NONE && readflag <= TCPSVR_READ_EVERY) {
        svr->read = readflag;
    } else {
        svr->read = TCPSVR_READ_ONLYONE;
    }
    svr->idx = 0;
    return 0;
}

int tcpsvr_open(struct tcpsvr *svr, const char *addr, int port)
{
    if (svr->socket != INVALID_WSOCKET) {
        return -1;
    }
    char portbuf[32];
    snprintf(portbuf, sizeof(portbuf), "%d", port);
    svr->socket = listen_on(addr, portbuf);
    if (svr->socket == INVALID_WSOCKET) {
        return -1;
    }
    return 0;
}

int tcpsvr_count_clients(struct tcpsvr *svr)
{
    int cnt = 0;
    for (int i = 0; i < TCPSVR_MAX_CLI; i++) {
        if (svr->clients[i] != INVALID_WSOCKET) {
            cnt++;
        }
    }
    return cnt;
}

static int tcpsvr_wait(struct tcpsvr *svr)
{
    wsocket sock = accept(svr->socket, NULL, NULL);
    if (sock == INVALID_WSOCKET && wsocket_errno != WSOCKET_EAGAIN) {
        return -1;
    }
    if (sock == INVALID_WSOCKET) {
        return 0;
    }
    int idx = -1;
    for (int i = 0; i < TCPSVR_MAX_CLI; i++) {
        if (svr->clients[i] == INVALID_WSOCKET) {
            idx = i;
            break;
        }
    }
    if (idx == -1) {
        wsocket_close(sock);
        return 0;
    }
    wsocket_set_nonblocking(sock);
    svr->clients[idx] = sock;
    return 0;
}

static int socket_recv(wsocket socket, void *buff, size_t count)
{
    if (socket == INVALID_WSOCKET) {
        return 0;
    }
    int rv = recv(socket, buff, count, 0);
    if (rv == WSOCKET_ERROR && wsocket_errno != WSOCKET_EAGAIN) {
        return -1;
    }
    if (rv == 0) {
        return -1;
    }
    return rv < 0 ? 0 : rv;
}

static int socket_send(wsocket socket, const void *data, size_t count)
{
    if (socket == INVALID_WSOCKET) {
        return 0;
    }
    int rv = send(socket, data, count, 0);
    if (rv == WSOCKET_ERROR && wsocket_errno != WSOCKET_EAGAIN) {
        return -1;
    }
    return rv < 0 ? 0 : rv;
}

int tcpsvr_read(struct tcpsvr *svr, void *buff, size_t count)
{
    if (tcpsvr_wait(svr) == -1) {
        return -1;
    }
    if (svr->read == TCPSVR_READ_EVERY) {
        int cnt= 0;
        wsocket *sock = NULL;
        while (cnt < TCPSVR_MAX_CLI) {
            if (svr->clients[svr->idx % TCPSVR_MAX_CLI] != INVALID_WSOCKET) {
                sock = &svr->clients[svr->idx % TCPSVR_MAX_CLI];
                svr->idx++;
                break;
            }
            svr->idx++;
            cnt++;
        }
        if (sock) {
            int rv = socket_recv(*sock, buff, count);
            if (rv == -1) {
                wsocket_close(*sock);
                *sock = INVALID_WSOCKET;
            }
            return rv <= 0 ? 0 : rv;
        }
        return 0;
    } else {
        unsigned char dummy[256];
        int first = 0;
        int rv = 0;
        for (int i = 0; i < TCPSVR_MAX_CLI; i++) {
            wsocket *sock = &svr->clients[i];
            if (*sock != INVALID_WSOCKET) {
                if (svr->read == TCPSVR_READ_ONLYONE && first == 0) {
                    rv = socket_recv(*sock, buff, count);
                    if (rv == -1) {
                        wsocket_close(*sock);
                        *sock = INVALID_WSOCKET;
                        rv = 0;
                    }
                } else {
                    int r = socket_recv(*sock, dummy, sizeof(dummy));
                    if (r == -1) {
                        wsocket_close(*sock);
                        *sock = INVALID_WSOCKET;
                    }
                }
            }
        }
        return rv;
    }
}

int tcpsvr_write(struct tcpsvr *svr, const void *data, size_t count)
{
    if (tcpsvr_wait(svr) == -1) {
        return -1;
    }
    for (int i = 0; i < TCPSVR_MAX_CLI; i++) {
        if (socket_send(svr->clients[i], data, count) == -1) {
            wsocket_close(svr->clients[i]);
            svr->clients[i] = INVALID_WSOCKET;
        }
    }
    return count;
}

int tcpsvr_close(struct tcpsvr *svr)
{
    if (svr) {
        if (svr->socket != INVALID_WSOCKET) {
            wsocket_close(svr->socket);
            svr->socket = INVALID_WSOCKET;
        }
        for (int i = 0; i < TCPSVR_MAX_CLI; i++) {
            if (svr->clients[i] != INVALID_WSOCKET) {
                wsocket_close(svr->clients[i]);
                svr->clients[i] = INVALID_WSOCKET;
            }
        }
    }
    return 0;
}

