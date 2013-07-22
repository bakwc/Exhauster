
add_library (utils string.cpp)
target_link_libraries(utils stuff
                            ${Boost_LIBRARIES}
                            ${ICONV_LIBRARY})
