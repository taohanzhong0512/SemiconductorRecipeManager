include(FetchContent)

message(STATUS "Fetching toml++...")
FetchContent_Declare(
        tomlplusplus
        GIT_REPOSITORY https://github.com/marzer/tomlplusplus.git
        GIT_TAG        v3.3.0
)

FetchContent_MakeAvailable(tomlplusplus)
