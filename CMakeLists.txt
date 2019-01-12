cmake_minimum_required(VERSION 2.8.8)

set(PROJECT_NAME reflection_generator)
project(${PROJECT_NAME})

add_executable(${PROJECT_NAME} 
    src/main.cpp
    src/generate_method.cpp
    src/generate_method.h)

# Link Boost
find_package(Boost REQUIRED COMPONENTS filesystem program_options)
message(STATUS "Found Boost version: ${Boost_VERSION}")
message(STATUS "Boost libs: ${Boost_LIBRARIES}")
message(STATUS "Boost includes: ${Boost_INCLUDE_DIRS}")

target_include_directories(${PROJECT_NAME} PRIVATE ${Boost_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} ${Boost_LIBRARIES})

# Link LLVM
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

find_package(LLVM REQUIRED CONFIG)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")
message(STATUS "LLVM libs: ${LLVM_LIBRARIES}")
message(STATUS "LLVM includes: ${LLVM_INCLUDE_DIRS}")

include_directories(${LLVM_INCLUDE_DIRS})
if(LLVM_BUILD_MAIN_SRC_DIR)
    target_include_directories(${PROJECT_NAME} ${LLVM_BUILD_MAIN_SRC_DIR}/tools/clang/include)
    target_include_directories(${PROJECT_NAME} ${LLVM_BUILD_BINARY_DIR}/tools/clang/include)
endif()

foreach(LIBDIR IN LISTS LLVM_LIBRARY_DIRS)
    target_link_libraries(${PROJECT_NAME} -L${LLVM_LIBRARY_DIRS})
endforeach()

target_link_libraries(${PROJECT_NAME} ${LLVM_DEFINITIONS})

target_link_libraries(${PROJECT_NAME}
    LLVMTarget
    LLVMOption
)

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
    foreach(LIB IN LISTS LIBRARY_LIST)
        target_link_libraries(${PROJECT_NAME} optimized ${LIB})
        target_link_libraries(${PROJECT_NAME} debug     ${LIB}d)
    endforeach()
    target_compile_options(${PROJECT_NAME} PRIVATE ${CMAKE_CXX_FLAGS} /std=c++17)
else()
    target_link_libraries(${PROJECT_NAME} ${LIBRARY_LIST})
    target_compile_options(${PROJECT_NAME} PRIVATE ${CMAKE_CXX_FLAGS} -std=c++17)
endif()


