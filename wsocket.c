#include "wsocket.h"

#ifdef _WIN32
/* winsock */
#include <Windows.h>

int wsocket_lib_init()
{
    WSADATA wsadata = {0};
    return WSAStartup(MAKEWORD(2, 0), &wsadata);
}

void wsocket_lib_cleanup()
{
    return WSACleanup();
}
#else
#include <fcntl.h>
#endif


static int wsocket_configure_blocking(wsocket sock, int blocking)
{
#ifdef _WIN32
   unsigned long mode = blocking ? 0 : 1;
   return ioctlsocket(sock, FIONBIO, &mode);
#else
   int flags = fcntl(sock, F_GETFL, 0);
   if (flags == -1) {
       return WSOCKET_ERROR;
   }
   flags = blocking ? (flags & ~O_NONBLOCK) : (flags | O_NONBLOCK);
   return fcntl(sock, F_SETFL, flags);
#endif
}

int wsocket_set_nonblocking(wsocket sock)
{
    return wsocket_configure_blocking(sock, 1);
}

int wsocket_set_blocking(wsocket sock)
{
    return wsocket_configure_blocking(sock, 0);
}



