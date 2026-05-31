# src/common/CMakeLists.txt
add_library(CommonLib STATIC
        Logger.cpp
        ConfigManager.cpp
)

target_link_libraries(CommonLib
        PUBLIC
        Qt5::Core
        spdlog::spdlog
        tomlplusplus::tomlplusplus
)

target_include_directories(CommonLib
        PUBLIC
        ${CMAKE_SOURCE_DIR}/include
)