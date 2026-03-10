cmake_minimum_required(VERSION 3.2 FATAL_ERROR)
project("NlogN Proxy Server"
    VERSION
        0.1
    DESCRIPTION
        "Proxy server for serving NlogN/Waffle"
    LANGUAGES
        CXX
    )

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake-modules")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/../bin)


set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH}" "${PROJECT_SOURCE_DIR}/cmake")

# Dependencies
include(Dependencies)


add_executable( 
    nln_server
    src/nln_server.cpp
    src/proxy_constants.cpp
    src/proxy_constants.h
    src/proxy_types.cpp
    src/proxy_types.h
    src/waffle_thrift.cpp
    src/waffle_thrift.h
    src/waffle_thrift_response.cpp
    src/waffle_thrift_response.h
)
add_dependencies(nln_server thrift_ep)
target_link_libraries(
    nln_server
    ${THRIFTNB_LIBRARY}
    ${THRIFT_LIBRARY}

        ${LIBEVENT_LIBRARY}
    )
