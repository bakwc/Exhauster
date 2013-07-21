
add_library (server server.cpp)
target_link_libraries(server utils
                             mongoose
                             ${Boost_LIBRARIES})
