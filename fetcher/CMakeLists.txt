
add_library (fetcher fetcher.cpp)
target_link_libraries(fetcher utils
                             ${Boost_LIBRARIES}
                             ${CURL_LIBRARY})
