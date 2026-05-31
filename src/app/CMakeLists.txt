add_executable(${PROJECT_NAME} main.cpp)

target_link_libraries(${PROJECT_NAME}
        PRIVATE
        Qt5::Core
        Qt5::Network
        spdlog::spdlog
        tomlplusplus::tomlplusplus
        CommonLib
        CoreLib
        GatewayLib
        ControllerLib
        ProtoLib
)