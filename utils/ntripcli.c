#include "ntripcli.h"
#include <stdlib.h>
#include <stdio.h>

static const unsigned char base64_table[65] =
	"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static char *
base64_encode(const unsigned char *src, size_t src_len,
			  char *buf, size_t buf_len)
{
	unsigned char *out, *pos;
	const unsigned char *end, *in;
	size_t olen;
	int line_len;

	olen = src_len * 4 / 3 + 4; /* 3-byte blocks to 4-byte */
	olen += olen / 72; /* line feeds */
	olen++; /* nul termination */
	if (olen < src_len)
		return NULL; /* integer overflow */
	if (buf_len < olen) {
	    return NULL; // out buf not enough
	}
	out = (unsigned char *)buf;

	end = src + src_len;
	in = src;
	pos = out;
	line_len = 0;
	while (end - in >= 3) {
		*pos++ = base64_table[in[0] >> 2];
		*pos++ = base64_table[((in[0] & 0x03) << 4) | (in[1] >> 4)];
		*pos++ = base64_table[((in[1] & 0x0f) << 2) | (in[2] >> 6)];
		*pos++ = base64_table[in[2] & 0x3f];
		in += 3;
		line_len += 4;
		if (line_len >= 72) {
			*pos++ = '\n';
			line_len = 0;
		}
	}

	if (end - in) {
		*pos++ = base64_table[in[0] >> 2];
		if (end - in == 1) {
			*pos++ = base64_table[(in[0] & 0x03) << 4];
			*pos++ = '=';
		} else {
			*pos++ = base64_table[((in[0] & 0x03) << 4) |
					      (in[1] >> 4)];
			*pos++ = base64_table[(in[1] & 0x0f) << 2];
		}
		*pos++ = '=';
		line_len += 4;
	}

	if (line_len)
		*pos++ = '\n';

	*pos = '\0';
	return out;
}


enum NtripCliStep {
    STEP_NEW = 0,
    STEP_CONN = 1,
    STEP_EXPECT = 2,
    STEP_DONE = 3,
};

int ntripcli_init(struct ntripcli *ntrip,
                  float conn_timeout,
                  float inact_timeout,
                  float reconn_wait)

{
    ntrip->user[0] = '\0';
    ntrip->passwd[0] = '\0';
    ntrip->mnt[0] = '\0';
    ntrip->token_cache[0] = '\0';
    ntrip->step = STEP_NEW;
    ntrip->cache[0] = '\0';
    ntrip->cache_idx = 0;
    ntrip->path_cache[0] = '\0';

    return tcpcli_init(&ntrip->tcp, conn_timeout, inact_timeout, reconn_wait);
}

int ntripcli_open(struct ntripcli *ntrip,
                  const char *addr, int port,
                  const char *user, const char *passwd,
                  const char *mnt)
{
    snprintf(ntrip->user ,sizeof(ntrip->user), "%s", user);
    snprintf(ntrip->passwd, sizeof(ntrip->passwd), "%s", passwd);
    snprintf(ntrip->mnt, sizeof(ntrip->mnt), "%s", mnt);
    char token[64];
    snprintf(token, sizeof(token), "%s:%s", user, passwd);
    if (base64_encode(token, strlen(token), ntrip->token_cache, sizeof(ntrip->token_cache)) == NULL) {
        return -1;
    }
    snprintf(ntrip->path_cache, sizeof(ntrip->path_cache), "%s:%s@%s:%d/%s",
             user, passwd, addr, port, mnt);
    int rv = tcpcli_open(&ntrip->tcp, addr, port);
    if (rv == -1) {
        return -1;
    }
    ntrip->step = STEP_CONN;
    return 0;
}

int ntripcli_open_path(struct ntripcli *ntrip, const char *path)
{
    char buf[256];
    snprintf(buf, sizeof(buf), "%s", path);
    char *passwd = strchr(buf, ':');
    if (passwd == NULL) {
        return - 1;
    }
    char *addr = strrchr(buf, '@');
    if (addr == NULL) {
        return -1;
    }
    char *port = strchr(addr, ':');
    if (port == NULL) {
        return -1;
    }
    char *mnt = strchr(port, '/');
    if (mnt == NULL) {
        return -1;
    }
    *passwd = '\0';
    passwd += 1;
    *addr = '\0';
    addr += 1;
    *port = '\0';
    port += 1;
    *mnt = '\0';
    mnt += 1;

    return ntripcli_open(ntrip, addr, atoi(port), buf, passwd, mnt);
}


// return: -1 = error, 0 = wait, 1 = ok
static int ntripcli_wait(struct ntripcli *ntrip)
{
    if (ntrip->step == STEP_NEW) {
        return -1;
    }
    if (!tcpcli_isconnected(&ntrip->tcp)) {
        ntrip->step = STEP_CONN;
        ntrip->cache[0] = '\0';
        ntrip->cache_idx = 0;
    }
    if (ntrip->step == STEP_CONN) {
        char buf[256];
        snprintf(buf, sizeof(buf),
                 "GET /%s HTTP/1.0\r\n"
                 "User-Agent: https://github.com/lazytinker/wsocket\r\n"
                 "Authorization: Basic %s\r\n"
                 "\r\n",
                 ntrip->mnt, ntrip->token_cache);
        int rv = tcpcli_write(&ntrip->tcp, buf, strlen(buf));
        if (rv == -1) {
            return -1;
        }
        if (rv == strlen(buf)) {
            ntrip->step = STEP_EXPECT;
        }
    }
    if (ntrip->step == STEP_EXPECT) {
        int rd = tcpcli_read(&ntrip->tcp,
                             ntrip->cache + ntrip->cache_idx,
                             sizeof(ntrip->cache) - ntrip->cache_idx - 1);
        if (rd == -1) {
            return -1;
        }
        if (rd > 0) {
            ntrip->cache_idx += rd;
            ntrip->cache[ntrip->cache_idx] = '\0';
            if (strstr(ntrip->cache, "ICY 200 OK\r\n")) {
                ntrip->step = STEP_DONE;
                ntrip->cache[0] = '\0';
                ntrip->cache_idx = 0;
            } else if (strstr(ntrip->cache, "HTTP/")){
                ntrip->step = STEP_CONN;
            } else if (ntrip->cache_idx >= sizeof(ntrip->cache)) {
                ntrip->step = STEP_CONN;
            }
        }
    }
    if (ntrip->step == STEP_DONE) {
        return 1;
    }
    return 0;
}


int ntripcli_read(struct ntripcli *ntrip, void *buff, size_t count)
{
    int rv = ntripcli_wait(ntrip);
    if (rv < 0) {
        return -1;
    } else if (rv == 0) {
        return 0;
    } else {
        return tcpcli_read(&ntrip->tcp, buff, count);
    }
}

int ntripcli_write(struct ntripcli *ntrip, const void *data, size_t count)
{
    int rv = ntripcli_wait(ntrip);
    if (rv < 0) {
        return -1;
    } else if (rv == 0) {
        return 0;
    } else {
        return tcpcli_write(&ntrip->tcp, data, count);
    }
}

int ntripcli_close(struct ntripcli *ntrip)
{

    if (ntrip) {
        tcpcli_close(&ntrip->tcp);
        ntrip->user[0] = '\0';
        ntrip->passwd[0] = '\0';
        ntrip->mnt[0] = '\0';
        ntrip->token_cache[0] = '\0';
        ntrip->step = STEP_NEW;
        ntrip->cache[0] = '\0';
        ntrip->cache_idx = 0;
        ntrip->path_cache[0] = '\0';
    }
    return 0;
}


