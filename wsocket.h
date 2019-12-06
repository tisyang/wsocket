#ifndef W_SOCKET_H
#define W_SOCKET_H

// socket api wrapper, for win and linux
// author by TyK
// Github: https://github.com/lazytinker/wsocket

#ifdef __cplusplus
#define WSOCKET_API  extern "C"
#else
#define WSOCKET_API
#endif // __cplusplus


#ifdef _WIN32
// windows winsock api
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <io.h>

typedef SOCKET wsocket;
#define INVALID_WSOCKET     INVALID_SOCKET
#define wsocket_errno       (WSAGetLastError())
#define WSOCKET_ERROR       SOCKET_ERROR
#define WSOCKET_EWOULDBLOCK WSAEWOULDBLOCK
#define WSOCKET_EAGAIN      WSOCKET_EWOULDBLOCK
#define WSOCKET_EINPROGRESS WSAEWOULDBLOCK

// call this to init wsocket library.Setup WSA on win, do nothing on linux.
#define WSOCKET_INIT()      wsocket_lib_init()
#define WSOCKET_CLEANUP()   wsocket_lib_cleanup()

WSOCKET_API int wsocket_lib_init();
WSOCKET_API int wsocket_lib_cleanup();

#define wsocket_close(s)    closesocket(s)
WSOCKET_API char* wsocket_strerror(int err);

#define WSOCKET_GET_FD(wsock)   _open_osfhandle(wsock, 0)

#else
// linux socket api
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

typedef int wsocket;
#define INVALID_WSOCKET     (-1)
#define wsocket_errno       (errno)
#define WSOCKET_ERROR       (-1)
#define WSOCKET_EWOULDBLOCK EWOULDBLOCK
#define WSOCKET_EAGAIN      WSOCKET_EWOULDBLOCK
#define WSOCKET_EINPROGRESS EINPROGRESS

// call this to init wsocket library. Setup WSA on win, do nothing on linux.
#define WSOCKET_INIT()
// call this to clean up wsocket library. Cleanup WSA on win, do nothing on linux.
#define WSOCKET_CLEANUP()

#define wsocket_close(s)    close(s)
#define wsocket_strerror(e) strerror(e)

#define WSOCKET_GET_FD(wsock)   (wsock)

#endif


// enable non-blocking on socket. return 0 on successfully, otherwise return
// WSOCKET_ERROR, and check wsocket_errno for details.
WSOCKET_API int wsocket_set_nonblocking(wsocket sock);

// enable blocking on socket. return 0 on successfully, otherwise return
// WSOCKET_ERROR, and check wsocket_errno for details.
WSOCKET_API int wsocket_set_blocking(wsocket sock);


#endif /* W_SOCKET_H */

