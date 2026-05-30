cmake_minimum_required(VERSION 3.30)
project(SemiconductorRecipeManager VERSION 1.0.0 LANGUAGES CXX)

# 1.设置c++17标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# 2. 设置 Qt5 路径

if(DEFINED ENV{QT_PATH})
    set(CMAKE_PREFIX_PATH "$ENV{QT_PATH}")
    message(STATUS "Found Qt5 via ENV QT_PATH: $ENV{QT_PATH}")
elseif(EXISTS "C:/Application/Qt/5.15.2/msvc2019_64")
    # 如果环境变量没设置，直接硬编码你本地的实际路径
    set(CMAKE_PREFIX_PATH "C:/Application/Qt/5.15.2/msvc2019_64")
    message(STATUS "Found Qt5 via hardcoded path: C:/Application/Qt/5.15.2/msvc2019_64")
else()
    message(FATAL_ERROR "Cannot find Qt5! Please set CMAKE_PREFIX_PATH to your Qt 5.15.2 directory.")
endif()

find_package(
        Qt5 REQUIRED COMPONENTS
        Core
        Gui
        Quick
        Network
)

# 3.开启Qt自动化
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# 4.引入自定义CMake脚本
include(cmake/FetchSpdlog.cmake)
include(cmake/FetchTomlPP.cmake)

find_package(Protobuf REQUIRED)

add_subdirectory(proto)
add_subdirectory(src)
