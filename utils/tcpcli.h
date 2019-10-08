#ifndef TCPCLI_H
#define TCPCLI_H

#include "../wsocket.h"

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

    float connect_timeout;  // connection timeout, in seconds. <= 0 means system specific.
    float inactive_timeout; // recv inactive timeout, in seconds. <= 0 means forever.
    float reconnect_wait;   // wait time before start reconnect. 0 means no wait.
                            // < 0 means wait forever, this makes tcpcli one shot connection.
};

// init tcpcli object
// always return 0
int tcpcli_init(struct tcpcli *tcp, float conn_timeout, float inact_timeout, float reconn_wait);

// open tcpcli object, to connect to remote addr:port
// return 0 in success, -1 in error, it will not auto reconnect when return error.
// it will connect remote in non-blocking mode, and
// timeout limit is connect_timeout
int tcpcli_open(struct tcpcli *tcp, const char *addr, int port);

// check if tcpcli object is connected
// return 1 means connected, otherwise 0 not connected
int tcpcli_isconnected(struct tcpcli *tcp);

// read data from tcpcli object, in non-blocking mode.
// return -1 in error, otherwise return bytes count has read.
// it will auto reconnect in connection error and not return -1 if reconnect_wait >= 0.
// if reconnect_wait < 0, it will return -1 either connection error or in wating
int tcpcli_read(struct tcpcli *tcp, void *buff, size_t count);

// write data to tcpcli object, in non-blocking mode
// return -1 in error, otherwise return bytes count has written
// it will auto reconnect in connection error and not return -1 if reconnect_wait >= 0.
// if reconnect_wait < 0, it will return -1 either connection error or in wating
int tcpcli_write(struct tcpcli *tcp, const void *data, size_t count);


// get elpased seconds since last activity, < 0 means error.
// activity means state change or has read some data.
double tcpcli_last_activity(struct tcpcli *tcp);

// close tcpcli object
// always return 0
int tcpcli_close(struct tcpcli *tcp);

#ifdef __cplusplus
}
#endif


#endif // TCPCLI_H
