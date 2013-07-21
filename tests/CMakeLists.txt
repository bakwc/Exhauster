
add_executable(run_tests tests.cpp)
target_link_libraries(run_tests
                        htmlcxx
                        utils
                        server
                        libexhaust
                        ${Boost_LIBRARIES}
                        ${GTEST_LIBRARIES}
                        pthread)
