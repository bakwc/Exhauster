if(Boost_FOUND)
    add_executable (exhauster main.cpp fetcher.hpp)
    target_link_libraries(exhauster
                            htmlcxx
                            utils
                            libexhaust
                            ${Boost_LIBRARIES})
endif()
