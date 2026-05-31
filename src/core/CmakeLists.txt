# src/core/CMakeLists.txt
add_library(CoreLib STATIC
        RecipeManager.cpp
)

target_link_libraries(CoreLib
        PUBLIC
        Qt5::Core
        ProtoLib
        CommonLib
)

target_include_directories(CoreLib
        PUBLIC
        ${CMAKE_SOURCE_DIR}/include
)