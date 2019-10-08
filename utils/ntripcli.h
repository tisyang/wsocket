#ifndef NTRIPCLI_H
#define NTRIPCLI_H

#include <stddef.h>
#include "tcpcli.h"
#ifdef __cplusplus
extern "C" {
#endif

struct ntripcli {
    struct tcpcli tcp;

    int step;
    char user[32];
    char passwd[32];
    char mnt[32];
    char token_cache[64];
    char path_cache[256];

    unsigned char cache[512];
    size_t cache_idx;
};


// init ntripcli object
// alway return 0
// conn_timeout/inact_timeout/recon_wait: see struct tcpcli.
int ntripcli_init(struct ntripcli *ntrip,
                  float conn_timeout,
                  float inact_timeout,
                  float reconn_wait);

// open ntripcli object
// return -1 in error, 0 in success.
// it will connect in non-blocking, and timeout limit is conn_timeout.
int ntripcli_open(struct ntripcli *ntrip,
                  const char *addr, int port,
                  const char *user, const char *passwd,
                  const char *mnt);

// same as ntripcli_open, path is in format "user:passwd@addr:port/mnt"
int ntripcli_open_path(struct ntripcli *ntrip, const char *path);


// read data from ntripcli object, in non-blocking mode.
// return -1 in error, otherwise return bytes count has read.
// it will auto reconnect in connection error and not return -1 if reconn_wait >= 0.
// if reconn_wait < 0, it will return -1 either connection error or in waiting.
int ntripcli_read(struct ntripcli *ntrip, void *buff, size_t count);

// write data to ntripcli object, in non-blocking mode.
// return -1 in error ,otherwise return bytes count has written.
// it will auto reconnect in connection error and not return -1 if reconn_wait >= 0.
// if reconn_wait < 0, it will return -1 either connection error or in waiting.
int ntripcli_write(struct ntripcli *ntrip, const void *data, size_t count);

// close ntripcli object
// always return 0
int ntripcli_close(struct ntripcli *ntrip);



#ifdef __cplusplus
}
#endif
#endif // NTRIPCLI_H

