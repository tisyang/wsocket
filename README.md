wsocket - Wrapper of Socket Programming
=======================================

wsocket is a simple wrapper of socket API functions.

It support linux and windows(Mingw-64 OK). It is just a thin wrapper, there is no self defined data structures, so user can use it with event libraries like libev.

## Build
This repo is a CMake subdirectory, just place it into your project, and add line to your `CMakeLists.txt`

```cmake
# if build utils
set(WSOCKET_BUILD_UTILS ON)
add_subdirectory(wsocket)

# link to wsocket
add_xxx(...)
target_link_libraries(... wsocket)
```
## Usage

See `wsocket.h` and `utils` code.

## Utils

1. `ntripcli`. A simple implementation of ntrip client.
2. `tcpcli`. A simple implementation of tcp client.
3. `tcpsvr`. A simple implementation of tcp server.

## LICENSE
BSD-3 Clause

