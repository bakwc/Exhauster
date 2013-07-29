
add_executable (exhauster main.cpp)
target_link_libraries(exhauster
                        htmlcxx
                        utils
                        libexhaust
                        server
                        fetcher
                        stuff
                        json
                        web
                        cpptemplate
                        ${Boost_LIBRARIES})
