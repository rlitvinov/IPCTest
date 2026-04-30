cmake_minimum_required(VERSION 3.16)

set(CMAKE_VERBOSE_MAKEFILE ON)

project(IPCTest LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


set(IPCTEST_COMMON_INCLUDE ${PROJECT_SOURCE_DIR}/IPCTestCommon/include)
set(IPCTEST_COMMON_INCLUDE_FILES ${IPCTEST_COMMON_INCLUDE}/common.h
                                 ${IPCTEST_COMMON_INCLUDE}/queue.h
                                 ${IPCTEST_COMMON_INCLUDE}/shared_queue.h)

add_subdirectory(IPCTestProducer)
add_subdirectory(IPCTestConsumer)