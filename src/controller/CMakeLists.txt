# src/controller/CMakeLists.txt
add_library(ControllerLib STATIC
    MainController.cpp
)

target_link_libraries(ControllerLib
    PUBLIC
    Qt5::Core
    CoreLib
    GatewayLib
    CommonLib
)

target_include_directories(ControllerLib
    PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)