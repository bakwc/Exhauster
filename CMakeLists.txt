cmake_minimum_required(VERSION 2.6)
project(exhauster)

set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)
find_package(Boost 1.48.0 COMPONENTS locale)

include_directories (${PROJECT_SOURCE_DIR})
if(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
endif()

add_subdirectory(utils)
add_subdirectory(libexhaust)
add_subdirectory(contrib)
add_subdirectory(exhauster)
