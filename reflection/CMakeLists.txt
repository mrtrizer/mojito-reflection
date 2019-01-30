cmake_minimum_required(VERSION 2.8.8)

set(PROJECT_NAME MojitoReflection)
project(${PROJECT_NAME})

file(GLOB PROJECT_SOURCES "${PROJECT_SOURCE_DIR}/src/*.hpp" "${PROJECT_SOURCE_DIR}/src/*.cpp")
add_library(${PROJECT_NAME} ${PROJECT_SOURCES})

target_compile_options(${PROJECT_NAME} PRIVATE ${CMAKE_CXX_FLAGS} -std=c++17)

target_include_directories(${PROJECT_NAME} PUBLIC ./src/)
