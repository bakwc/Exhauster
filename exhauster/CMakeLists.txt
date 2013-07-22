
add_executable (exhauster main.cpp)
target_link_libraries(exhauster
                        htmlcxx
                        utils
                        libexhaust
                        server
                        fetcher
                        libstuff
                        ${Boost_LIBRARIES})
