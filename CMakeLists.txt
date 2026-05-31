cmake_minimum_required(VERSION 3.30)
project(SemiconductorRecipeManager VERSION 1.0.0 LANGUAGES CXX)
if(MSVC)
    # 源文件和执行字符集都用UTF-8
    add_compile_options(/utf-8)
    add_compile_options(/diagnostics:utf8)  # ← 新增这一行
    # 强制ninja构建时的子进程代码页为UTF-8
    set(ENV{VSLANG} 1033)  # 强制MSVC用英文输出诊断信息
endif()
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

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
set(protobuf_DIR "D:/tool/vcpkg/installed/x64-windows/share/protobuf")
set(absl_DIR "D:/tool/vcpkg/installed/x64-windows/share/absl")
set(utf8_range_DIR "D:/tool/vcpkg/installed/x64-windows/share/utf8_range") # 🌟 新增

# 2. 🌟 关键修复：按依赖顺序查找 (utf8_range -> absl -> protobuf)
find_package(utf8_range CONFIG REQUIRED)  # 🌟 新增
find_package(absl CONFIG REQUIRED)
find_package(protobuf CONFIG REQUIRED)


add_subdirectory(proto)
add_subdirectory(src)
