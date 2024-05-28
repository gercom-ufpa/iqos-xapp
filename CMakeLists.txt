cmake_minimum_required(VERSION 3.15)
project(iqos_xapp VERSION 0.0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)

## Add libs
add_subdirectory(libs)

## Config executable
add_executable(iqos_xapp src/main.cpp)

## Link libraries
target_include_directories(iqos_xapp PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/libs/flexric/src/xApp
)
target_link_libraries(iqos_xapp PUBLIC
        e42_xapp
        -pthread
        -lsctp
        -ldl
)