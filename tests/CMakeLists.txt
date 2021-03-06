cmake_minimum_required(VERSION 2.8.8)

set(CMAKE_C_COMPILER "${CMAKE_HOME_DIRECTORY}/compile_cc.sh")
set(CMAKE_CXX_COMPILER "${CMAKE_HOME_DIRECTORY}/compile_c++.sh")
set(CMAKE_XCODE_ATTRIBUTE_CC  "${CMAKE_C_COMPILER}")
set(CMAKE_XCODE_ATTRIBUTE_CXX "${CMAKE_CXX_COMPILER}")

set(PROJECT_NAME tests)
project(${PROJECT_NAME})

file(GLOB PROJECT_SOURCES "${PROJECT_SOURCE_DIR}/src/*.hpp" "${PROJECT_SOURCE_DIR}/src/*.cpp")

add_executable(${PROJECT_NAME} ${PROJECT_SOURCES})

target_compile_options(${PROJECT_NAME} PRIVATE ${CMAKE_CXX_FLAGS} -std=c++17)

# Link Boost
find_package(Boost REQUIRED COMPONENTS unit_test_framework)
message(STATUS "Found Boost version: ${Boost_VERSION}")
message(STATUS "Boost libs: ${Boost_LIBRARIES}")
message(STATUS "Boost includes: ${Boost_INCLUDE_DIRS}")

target_include_directories(${PROJECT_NAME} PRIVATE ${Boost_INCLUDE_DIRS})
target_include_directories(${PROJECT_NAME} PRIVATE catch)
target_link_libraries(${PROJECT_NAME} ${Boost_LIBRARIES})

# Link reflection
add_subdirectory("../reflection" "MojitoReflection/")
target_link_libraries(${PROJECT_NAME} MojitoReflection)
