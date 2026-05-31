半导体设备上位机软件开发实战指南
1. 项目概述与技术栈选型
   本项目旨在为半导体设备（如 EFEM、机械手等）开发一套高稳定性、高并发、前后端分离的现代工业级上位机控制软件。

开发语言：现代 C++ (C++17)
GUI 框架：Qt 5.15.2 (QML/Qt Quick 用于前端，C++ 用于后端)
构建系统：CMake + Visual Studio 17 2022 Generator (在 CLion 中开发)
依赖管理：vcpkg (用于管理 Protobuf 等复杂 C++ 库)
数据序列化：Protobuf (二进制协议，高效且跨语言)
配置管理：TOML 格式 (使用 toml++ 库，层级清晰，可读性强)
日志系统：spdlog (支持异步、多线程、按天滚动、彩色控制台输出)
2. 工业级目录架构设计
   项目采用严格的 接口与实现分离 (include/src) 以及 模块化分层 架构，确保高内聚、低耦合。

SemiconductorRecipeManager/
├── CMakeLists.txt                 # 顶层配置 (全局依赖查找、Qt/vcpkg配置)
├── cmake/                         # CMake 辅助脚本 (FetchContent 等)
├── config/                        # 默认配置文件 (TOML)
├── proto/                         # Protobuf 协议定义 (.proto)
│
├── include/                       # 🌟 公共接口目录 (对外暴露的 .h 文件)
│   ├── common/                    # 基础设施 (Logger, ConfigManager)
│   ├── core/                      # 核心业务 (RecipeManager)
│   ├── gateway/                   # 外部网关 (MesGateway)
│   └── controller/                # 协调层 (MainController)
│
└── src/                           # 🔒 私有实现目录 (内部 .cpp 和私有 .h)
├── app/                       # 应用程序入口 (main.cpp)
├── common/                    # 基础设施实现
├── core/                      # 核心业务实现
├── gateway/                   # 网关实现 (包含 TcpWorker, Protocol 等内部细节)
└── controller/                # 协调层实现
3. 核心模块设计与实现要点
   3.1 基础设施层 (Common)
   Logger (spdlog 封装)：
   拦截 Qt 原生的 qDebug/qWarning 并重定向到 spdlog。
   配置双 Sink：彩色控制台输出 + 每日滚动文件输出。
   ConfigManager (toml++ 封装)：
   采用单例模式，全局共享。
   支持嵌套 Key 读取（如 Network.Port），支持默认值。
   3.2 核心业务层 (Core)
   RecipeManager (配方管理)：
   使用 Protobuf 生成的 C++ 类 (Semiconductor::Recipe) 作为内存数据模型。
   提供 loadRecipeFromData (反序列化) 和 getSerializedRecipe (序列化) 接口，与底层协议解耦。
   3.3 外部网关层 (Gateway) - 核心难点
   协议设计 (Protocol.h)：
   自定义 8 字节二进制帧头：Magic (0x53454D49) + PayloadLen。
   使用 #pragma pack(1) 防止内存对齐问题。
   TcpWorker (粘包/半包处理)：
   在独立子线程中运行，维护 QByteArray m_buffer 接收缓冲区。
   核心逻辑：循环检查缓冲区 -> 校验帧头魔数 -> 检查 Payload 长度 -> 提取完整数据 -> 移除已处理字节。
   MesGateway (线程调度)：
   继承 QTcpServer，重写 incomingConnection。
   为每个新连接创建 TcpWorker 并使用 moveToThread 移入独立 QThread，保证高并发下不阻塞主线程。
   3.4 协调层 (Controller)
   MainController：
   作为中枢神经，通过 Qt 信号槽将 MesGateway 接收到的纯净二进制数据，安全地跨线程传递给 RecipeManager 进行业务处理。
4. CMake 与环境配置避坑指南 (血泪经验)
   4.1 vcpkg 与 Protobuf 依赖链
   新版 Protobuf (v22+) 强依赖 abseil 和 utf8_range。在 CMake 中必须在根目录按依赖顺序显式查找：

# 必须严格按照 底层依赖 -> 上层依赖 的顺序
find_package(utf8_range CONFIG REQUIRED)
find_package(absl CONFIG REQUIRED)
find_package(protobuf CONFIG REQUIRED) # 注意小写和 CONFIG 模式
4.2 手动调用 protoc 编译器
放弃 CMake 自带容易出 Bug 的 protobuf_generate_cpp 宏，使用 add_custom_command 手动调用 protoc.exe 生成 .pb.cc 和 .pb.h，可控性最强。

4.3 include/src 分离后的 Include 规范
在代码中包含头文件时，必须带上模块前缀，以避免路径冲突并明确依赖关系：

// 正确写法
#include "common/Logger.h"
#include "core/RecipeManager.h"
#include "gateway/MesGateway.h"

// 错误写法 (在分离架构下会找不到文件)
#include "Logger.h"
注：同一模块内部的私有头文件（如 MesGateway.cpp 包含 TcpWorker.h）可直接写文件名。

4.4 运行时 DLL 找不到问题
由于 vcpkg 默认安装动态库 (x64-windows)，运行前必须在 CLion/VS 的运行配置中，将 D:\tool\vcpkg\installed\x64-windows\bin 追加到环境变量 PATH 中。

5. 下一步开发计划 (Step 5 及以后)
   端到端闭环测试：
   编写 Python 脚本，构造带 8 字节帧头的 Protobuf 二进制流，通过 TCP 发送给 C++ 网关，验证粘包处理与反序列化链路。
   QML 前端开发：
   使用 QAbstractListModel 将 Protobuf 中的 repeated 字段适配为 QML ListView 的数据源。
   实现前后端数据绑定与交互。
   设备状态机引入：
   引入 QStateMachine 或状态模式，管理设备的 Init, Idle, Running, Error 等严格状态流转。
   SECS/GEM 协议对接 (进阶)：
   在 Gateway 层预留接口，未来将自定义 TCP 协议替换或扩展为半导体标准的 SECS/GEM (HSMS) 协议。
   结语： 从配置 CMake 和 vcpkg 的“泥潭”中杀出，到构建出支持多线程、粘包处理和 Protobuf 序列化的工业级骨架，你已经掌握了 C++ 上位机开发最核心的硬核技术。保持这种死磕到底的工程师精神，继续在半导体软件领域深耕吧！