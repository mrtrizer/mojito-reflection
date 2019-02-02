cmake_minimum_required(VERSION 2.8.8)

set(CMAKE_C_COMPILER "${CMAKE_HOME_DIRECTORY}/compile_cc.sh")
set(CMAKE_CXX_COMPILER "${CMAKE_HOME_DIRECTORY}/compile_c++.sh")
set(CMAKE_XCODE_ATTRIBUTE_CC "${CMAKE_C_COMPILER}")
set(CMAKE_XCODE_ATTRIBUTE_CXX "${CMAKE_CXX_COMPILER}")

set(PROJECT_NAME demo)
project(${PROJECT_NAME})

add_executable(${PROJECT_NAME} main.cpp)

target_compile_options(${PROJECT_NAME} PRIVATE ${CMAKE_CXX_FLAGS} -std=c++17)

# Link reflection
add_subdirectory("../reflection" "MojitoReflection/")
target_link_libraries(${PROJECT_NAME} PRIVATE MojitoReflection)

# Link demo library
add_subdirectory("demo_lib/")
target_link_libraries(${PROJECT_NAME} PRIVATE DemoLib)
