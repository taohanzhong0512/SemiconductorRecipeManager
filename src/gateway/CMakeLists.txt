add_library(GatewayLib STATIC
        MesGateway.cpp
        TcpWorker.cpp
)

target_link_libraries(GatewayLib
        PUBLIC
        Qt5::Core
        Qt5::Network
        CommonLib
)

# 🌟 关键：同时暴露 include 目录(对外) 和 当前 src 目录(对内)
target_include_directories(GatewayLib
        PUBLIC
        ${CMAKE_SOURCE_DIR}/include      # 让其他模块能找到 MesGateway.h
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}      # 让 MesGateway.cpp 能找到同目录下的 TcpWorker.h 和 Protocol.h
)
