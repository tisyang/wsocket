#include "wsocket.h"

#ifdef _WIN32
/* winsock */
#include <Windows.h>
#include <stdio.h>
#ifdef __MINGW32__
# define THREAD_LOCAL __thread
#else
# define THREAD_LOCAL __declspec(thread)
#endif

static THREAD_LOCAL char m_msgbuf[256];

int wsocket_lib_init()
{
    WSADATA wsadata = {0};
    return WSAStartup(MAKEWORD(2, 0), &wsadata);
}

int wsocket_lib_cleanup()
{
    return WSACleanup();
}

char* wsocket_strerror(int err)
{
    m_msgbuf[0] = '\0';
    wchar_t wbuf[256];
    wbuf[0] = L'\0';
    if (FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM |
                      FORMAT_MESSAGE_IGNORE_INSERTS,
                      NULL, err, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                      (LPTSTR)wbuf, sizeof(wbuf)/sizeof(wchar_t), NULL)) {
        wcstombs(m_msgbuf, wbuf, sizeof(m_msgbuf) - 1);
    } else {
        snprintf(m_msgbuf, sizeof(m_msgbuf), "error code '%d'", err);
    }
    return m_msgbuf;
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
    return wsocket_configure_blocking(sock, 0);
}

int wsocket_set_blocking(wsocket sock)
{
    return wsocket_configure_blocking(sock, 1);
}



