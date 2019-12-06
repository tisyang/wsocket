option(WSOCKET_BUILD_UTILS "Build utils for woskcet" ON)

aux_source_directory(utils SRC_UTILS)

if (WSOCKET_BUILD_UTILS)
add_library(wsocket wsocket.c wsocket.h ${SRC_UTILS})
else()
add_library(wsocket wsocket.c wsocket.h)
endif()

if (WIN32)
target_link_libraries(wsocket
    ws2_32
)
endif()

