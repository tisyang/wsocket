add_library(wsocket wsocket.c wsocket.h tcpcli.c tcpcli.h)

if (WIN32)
target_link_libraries(wsocket
    ws2_32
)
endif()

