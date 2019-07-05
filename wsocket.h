#ifndef W_SOCKET_H
#define W_SOCKET_H


#ifdef _WIN32
// windows winsock api
#include <WinSock2.h>
#include <WS2tcpip.h>

typedef SOCKET wsocket;
#define INVALID_WSOCKET     INVALID_SOCKET
#define wsocket_errno       (WSAGetLastError())
#define WSOCKET_ERROR       SOCKET_ERROR

// call this to init wsocket library.Setup WSA on win, do nothing on linux.
#define WSOCKET_INIT()      wsocket_lib_init()
#define WSOCKET_CLEANUP()   wsocket_lib_cleanup()

int wsocket_lib_init();
int wsocket_lib_cleanup();

#define wsocket_close(s)    closesocket(s)

#else
// linux socket api
#include <errno.h>

typedef int wsocket;
#define INVALID_WSOCKET     (-1)
#define wsocket_errno       (errno)
#define WSOCKET_ERROR       (-1)

// call this to init wsocket library. Setup WSA on win, do nothing on linux.
#define WSOCKET_INIT()
// call this to clean up wsocket library. Cleanup WSA on win, do nothing on linux.
#define WSOCKET_CLEANUP()

#define wsocket_close(s)    close(s)

#endif


// enable non-blocking on socket. return 0 on successfully, otherwise return
// WSOCKET_ERROR, and check wsocket_errno for details.
int wsocket_set_nonblocking(wsocket sock);

// enable blocking on socket. return 0 on successfully, otherwise return
// WSOCKET_ERROR, and check wsocket_errno for details.
int wsocket_set_blocking(wsocket sock);


#endif /* W_SOCKET_H */

