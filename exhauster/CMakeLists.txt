link_directories (${PROJECT_BINARY_DIR}/utils
                  ${PROJECT_BINARY_DIR}/libexhaust
                  ${PROJECT_BINARY_DIR}/contrib/htmlcxx/html)

if(Boost_FOUND)
    add_executable (exhauster main.cpp)
    target_link_libraries(exhauster
                            utils
                            libexhaust
                            ${Boost_LIBRARIES})
endif()
