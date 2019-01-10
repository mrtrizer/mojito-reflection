cmake_minimum_required(VERSION 2.8.8)

set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "Configs" FORCE)
set(CMAKE_SUPPRESS_REGENERATION TRUE)

set(PROJECT_NAME reflection_generator)

set(LIBRARY_LIST
    clangFrontend
    clangSerialization
    clangDriver
    clangParse
    clangSema
    clangAnalysis
    clangEdit
    clangAST
    clangLex
    clangBasic
    clangTooling
    clangASTMatchers)

project(${PROJECT_NAME})

find_package(LLVM REQUIRED CONFIG)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

include_directories(${LLVM_INCLUDE_DIRS})
if(LLVM_BUILD_MAIN_SRC_DIR)
  include_directories(${LLVM_BUILD_MAIN_SRC_DIR}/tools/clang/include)
  include_directories(${LLVM_BUILD_BINARY_DIR}/tools/clang/include)
endif()
link_directories(${LLVM_LIBRARY_DIRS})
add_definitions(${LLVM_DEFINITIONS})

add_subdirectory(lib/tiny-process-library)

add_executable(${PROJECT_NAME} src/main.cpp src/generate_method.cpp src/generate_method.h)

target_include_directories(${PROJECT_NAME} PRIVATE lib/json)

target_link_libraries(${PROJECT_NAME} tiny-process-library)

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
  foreach(LIB IN LISTS LIBRARY_LIST)
    target_link_libraries(${PROJECT_NAME} optimized ${LIB})
    target_link_libraries(${PROJECT_NAME} debug     ${LIB}d)
  endforeach()
else()
  target_link_libraries(${PROJECT_NAME} ${LIBRARY_LIST})
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wno-unused-parameter -fno-strict-aliasing -fno-rtti")
endif()

target_link_libraries(${PROJECT_NAME}
  LLVMTarget
  LLVMOption # Support
)

message(STATUS "User selected librarys = ${LIBRARY_LIST}")
message(STATUS "    = ${llvm_libs}")
