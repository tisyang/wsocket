#ifndef TCPCLI_H
#define TCPCLI_H

#include "wsocket.h"

#ifdef __cplusplus
extern "C" {
#endif

// tcp client object
// use it to recv/send remote tcp server data
// it will auto reconnect if error detect
struct tcpcli {
    wsocket socket;

    int state;
    double activity;

    char addr[64];
    char serv[32];

    float connect_timeout;  // connection timeout, in seconds. <= 0 means forever.
    float inactive_timeout; // recv inactive timeout, in seconds. <= 0 means no forever.
    float reconnect_wait;   // wait time before start reconnect. <= 0 means no wait.
};

// init tcpcli object
// always return 0
int tcpcli_init(struct tcpcli *tcp, float conn_timeout, float inact_timeout, float reconn_wait);

// open tcpcli object, to connect to remote addr:port
// return -1 in error, 0 in success
// it will connect remote in non-blocking mode, and
// timeout limit is CONNECTION_TIMEOUT 
int tcpcli_open(struct tcpcli *tcp, const char *addr, int port);

// read data from tcpcli object, in non-blocking mode
// return -1 in error, otherwise return bytes count has read
// it will auto reconnect in error
int tcpcli_read(struct tcpcli *tcp, void *buff, size_t count);

// write data to tcpcli object, in non-blocking mode
// return -1 in error, otherwise return bytes count has written
// it will auto reconnect in error
int tcpcli_write(struct tcpcli *tcp, const void *data, size_t count);

// close tcpcli object
// always return 0
int tcpcli_close(struct tcpcli *tcp);

#ifdef __cplusplus
}
#endif


#endif // TCPCLI_H
