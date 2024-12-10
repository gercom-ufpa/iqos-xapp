enable_language(OBJC)
cmake_minimum_required(VERSION 3.15)
project(xapp_iqos VERSION 0.0.1 LANGUAGES CXX C ASM)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_C_FLAGS_RELEASE "-O3")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

## Enable OpenMP
find_package(OpenMP)
if(OpenMP_FOUND)
    message(STATUS "OpenMP found: ${OpenMP_CXX_FLAGS}")
    add_compile_definitions(_OPENMP)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
else()
    message(WARNING "OpenMP not found. Ensure it is installed for your compiler.")
endif()

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
        src/cellstr_sort.c
        src/elementwiseOperationInPlace.c
        src/find.c
        src/getCategoryNames.c
        src/insertionsort.c
        src/internal_softmax.c
        src/introsort.c
        src/postProcessOutputToReturnCategorical.c
        src/predict.c
        src/predictForRNN.c
        src/rtGetInf.c
        src/rtGetNaN.c
        src/rt_nonfinite.c
        src/runPrediction.c
        src/runPrediction_data.c
        src/runPrediction_initialize.c
        src/runPrediction_terminate.c
        src/SoftmaxLayer.c
        src/strcmp.c
        src/strtrim.c
)

## Link libraries
target_include_directories(xapp_iqos PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/libs
        ${CMAKE_CURRENT_SOURCE_DIR}/libs/flexric/src/xApp
)
target_link_libraries(xapp_iqos PUBLIC
        e42_xapp
        spdlog::spdlog $<$<BOOL:${MINGW}>:ws2_32>
        -pthread
        -lsctp
        -ldl
)
