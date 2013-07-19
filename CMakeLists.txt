cmake_minimum_required(VERSION 2.6)
project(exhauster)

set(CMAKE_CXX_FLAGS "-std=c++0x")

set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)

find_package(Boost 1.48.0 COMPONENTS locale)
find_package(GTest REQUIRED)

include_directories (${PROJECT_SOURCE_DIR})
include_directories(${Boost_INCLUDE_DIRS})
include_directories(${GTEST_INCLUDE_DIRS})

link_directories (${PROJECT_BINARY_DIR}/utils
                  ${PROJECT_BINARY_DIR}/libexhaust
                  ${PROJECT_BINARY_DIR}/contrib/htmlcxx/html
                  ${PROJECT_BINARY_DIR}/contrib/htmlcxx/mongoose)

add_subdirectory(utils)
add_subdirectory(libexhaust)
add_subdirectory(contrib)
add_subdirectory(exhauster)
add_subdirectory(tests)
