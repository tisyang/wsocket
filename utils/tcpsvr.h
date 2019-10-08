#ifndef TCPSVR_H
#define TCPSVR_H


#include "../wsocket.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// max clients count
#define TCPSVR_MAX_CLI  32

enum {
    TCPSVR_READ_NONE,    // none of clients data will be read, this is useful with
                         // tcpsvr_read to detect client disconnect.
    TCPSVR_READ_ONLYONE, // only one client(first valid client) data will be read
    TCPSVR_READ_EVERY,   // every client data will be read
};

struct tcpsvr {
    wsocket socket;
    wsocket clients[TCPSVR_MAX_CLI];
    int     read;   // TCPSVR_READ_XXX
    size_t  idx;
};


// init tcpsvr, always return 0.
int tcpsvr_init(struct tcpsvr *svr, int readflag);

// open tcpsvr, return 0 on success, -1 on error.
int tcpsvr_open(struct tcpsvr *svr, const char *addr, int port);

// return valid clients count
int tcpsvr_count_clients(struct tcpsvr *svr);

// read from tcpsvr in non-blocking mode.
int tcpsvr_read(struct tcpsvr *svr, void *buff, size_t count);

// write into tcpsvr.
int tcpsvr_write(struct tcpsvr *svr, const void *data, size_t count);

// close tcpsvr, always return 0.
int tcpsvr_close(struct tcpsvr *svr);


#ifdef __cplusplus
}
#endif
#endif // TCPSVR_H
