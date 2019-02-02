cmake_minimum_required(VERSION 2.8.8)

set(PROJECT_NAME DemoLib)
project(${PROJECT_NAME})

add_library(${PROJECT_NAME} test.cpp)

target_compile_options(${PROJECT_NAME} PRIVATE ${CMAKE_CXX_FLAGS} -std=c++17)

target_link_libraries(${PROJECT_NAME} PRIVATE MojitoReflection)
