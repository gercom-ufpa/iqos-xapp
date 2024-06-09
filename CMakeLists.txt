cmake_minimum_required(VERSION 3.15)
project(xapp_iqos VERSION 0.0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_C_FLAGS_RELEASE "-O3")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

## Definitions
add_compile_definitions(SPDLOG_ACTIVE_LEVEL)

## Add libs
add_subdirectory(libs)

## build external libs
### SPDLOG for logs
Include(FetchContent)
FetchContent_Declare(
        spdlog
        GIT_REPOSITORY https://github.com/gabime/spdlog.git
        GIT_TAG v1.14.1
)
FetchContent_MakeAvailable(spdlog)

## Config executable
add_executable(xapp_iqos src/main.cpp
        src/logger.cpp
        src/kpmManager.cpp
        src/e2Info.cpp
)

## Link libraries
target_include_directories(xapp_iqos PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/libs/flexric/src/xApp
)
target_link_libraries(xapp_iqos PUBLIC
        e42_xapp
        spdlog::spdlog $<$<BOOL:${MINGW}>:ws2_32>
        -pthread
        -lsctp
        -ldl
)