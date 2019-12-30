#ifndef UDPCLI_H
#define UDPCLI_H

#include "../wsocket.h"

#ifdef __cplusplus
extern "C" {
#endif

// tcp client object
// use it to recv/send remote tcp server data
// it will auto reconnect if error detect
struct udpcli {
    wsocket socket;

    int state;
    double activity;

    char addr[64];
    char serv[32];

    float inactive_timeout; // recv inactive timeout, in seconds. <= 0 means forever.
    float reconnect_wait;   // wait time before start reconnect. 0 means no wait.
                            // < 0 means wait forever, this makes udpcli one shot connection.
};

// init udpcli object
// always return 0
int udpcli_init(struct udpcli *tcp, float inact_timeout, float reconn_wait);

// open udpcli object, to connect to remote addr:port
// return 0 in success, -1 in error, it will not auto reconnect when return error.
int udpcli_open(struct udpcli *tcp, const char *addr, int port);

// read data from udpcli object, in non-blocking mode.
// return -1 in error, otherwise return bytes count has read.
// it will auto reconnect in connection error or inactive detect, and not return -1
// if reconnect_wait >= 0. If reconnect_wait < 0, it will return -1 either
// connection error or in wating
int udpcli_read(struct udpcli *tcp, void *buff, size_t count);

// write data to udpcli object, in non-blocking mode
// return -1 in error, otherwise return bytes count has written
// it will auto reconnect in connection error or inactive detect, and not return -1
// if reconnect_wait >= 0. If reconnect_wait < 0, it will return -1 either
// connection error or in wating
int udpcli_write(struct udpcli *tcp, const void *data, size_t count);

// get elpased seconds since last activity, < 0 means error.
// activity means state change or has read some data.
double udpcli_last_activity(struct udpcli *tcp);

// close udpcli object
// always return 0
int udpcli_close(struct udpcli *tcp);

#ifdef __cplusplus
}
#endif


#endif // UDPCLI_H
