include(FetchContent)

message(STATUS "Fetching spdlog...")
FetchContent_Declare(
        spdlog
        GIT_REPOSITORY https://github.com/gabime/spdlog.git
        GIT_TAG        v1.12.0  # 指定一个稳定的版本
)

# 仅下载并配置，不自动编译 (我们会在自己的 target 中链接)
FetchContent_MakeAvailable(spdlog)
