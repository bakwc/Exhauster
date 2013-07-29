cmake_minimum_required(VERSION 2.6)
project(exhauster)

set(CMAKE_CXX_FLAGS "-std=c++0x")

set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)

find_package(Boost 1.48.0 COMPONENTS locale system filesystem REQUIRED)
find_package(GTest REQUIRED)

FIND_PATH(CURL_INCLUDE_DIR NAMES curl/curl.h)
FIND_PATH(ICONV_INCLUDE_DIR NAMES iconv.h)

FIND_LIBRARY(CURL_LIBRARY NAMES curl)
FIND_LIBRARY(ICONV_LIBRARY NAMES iconv libiconv libiconv-2 c)


add_definitions(-DTEST_DATA_DIR="${PROJECT_SOURCE_DIR}/test_data/")

include_directories(${PROJECT_SOURCE_DIR}
                    ${Boost_INCLUDE_DIRS}
                    ${GTEST_INCLUDE_DIRS}
                    ${CURL_INCLUDE_DIR}
                    ${ICONV_INCLUDE_DIR}
                    ${PROJECT_BINARY_DIR})

link_directories (${PROJECT_BINARY_DIR}/utils
                  ${PROJECT_BINARY_DIR}/libexhaust
                  ${PROJECT_BINARY_DIR}/server
                  ${PROJECT_BINARY_DIR}/contrib/htmlcxx/html
                  ${PROJECT_BINARY_DIR}/contrib/mongoose
                  ${PROJECT_BINARY_DIR}/contrib/lib_json
                  ${PROJECT_BINARY_DIR}/contrib/json
                  ${PROJECT_BINARY_DIR}/contrib/cpptemplate
                  ${PROJECT_BINARY_DIR}/contrib/stuff)

add_subdirectory(contrib)
add_subdirectory(utils)
add_subdirectory(respacker)
add_subdirectory(libexhaust)
add_subdirectory(server)
add_subdirectory(fetcher)
add_subdirectory(web)
add_subdirectory(exhauster)
add_subdirectory(tests)
