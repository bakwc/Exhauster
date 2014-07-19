
add_custom_command(
    OUTPUT resources.cpp
    COMMAND ${PROJECT_BINARY_DIR}/respacker/respacker ${PROJECT_SOURCE_DIR}/web/root/
    DEPENDS ${PROJECT_SOURCE_DIR}/respacker/respacker.cpp
            ${PROJECT_SOURCE_DIR}/web/root/index.html
            ${PROJECT_SOURCE_DIR}/web/root/js/bootstrap.js
            ${PROJECT_SOURCE_DIR}/web/root/js/jquery.js
            ${PROJECT_SOURCE_DIR}/web/root/css/bootstrap.css)

add_library(web resources.cpp)

add_dependencies(web respacker)
