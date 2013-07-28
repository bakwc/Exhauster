
add_library (server server.cpp content_type.cpp)
target_link_libraries(server utils
                             mongoose
                             ${Boost_LIBRARIES})
