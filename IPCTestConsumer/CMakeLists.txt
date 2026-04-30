cmake_minimum_required(VERSION 3.16)

project(IPCTestConsumer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(${PROJECT_NAME} main.cpp ${IPCTEST_COMMON_INCLUDE_FILES}
    statistics_printer.h statistics_printer.cpp)

target_include_directories(${PROJECT_NAME} PRIVATE ${IPCTEST_COMMON_INCLUDE})
