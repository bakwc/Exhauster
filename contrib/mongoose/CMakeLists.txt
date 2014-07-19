
add_library (mongoose mongoose.c)
target_link_libraries(mongoose pthread ssl crypto)
