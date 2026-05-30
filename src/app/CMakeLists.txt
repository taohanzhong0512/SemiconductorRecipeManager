add_executable(${PROJECT_NAME} main.cpp)

target_link_libraries(${PROJECT_NAME}
        PRIVATE
        Qt5::Core
        spdlog::spdlog      # 链接 spdlog
        tomlplusplus::tomlplusplus # 链接 toml++
)