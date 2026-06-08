  # Task 1/2/3 实现教程：Protobuf + Gateway + Core

> **前置条件：** 确保 vcpkg 已安装 protobuf，CMake 能正常 `find_package(protobuf)`。
>
> **注意：** 以下代码均为 Demo 参考，请根据实际需求调整字段和逻辑。

---

## 0. 前置修复：CMake 配置补全

在开始之前，需要先修复项目中几个 CMake 文件的链接问题。

### 0.1 `src/CMakeLists.txt` — 取消注释所有子模块

```cmake
# src/CMakeLists.txt

# 1. 基础设施层
add_subdirectory(common)

# 2. 核心业务层
add_subdirectory(core)

# 3. 外部网关层
add_subdirectory(gateway)

# 4. 协调层
add_subdirectory(controller)

# 5. 应用入口 (必须最后)
add_subdirectory(app)
```

### 0.2 `src/app/CMakeLists.txt` — 链接所有模块库

```cmake
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
```

### 0.3 确认 `src/common/CMakeLists.txt` 存在

如果 `src/common/` 目录下没有 `CMakeLists.txt`，需要创建：

```cmake
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
```

### 0.4 创建 `src/core/CMakeLists.txt`

```cmake
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
```

### 0.5 创建 `src/controller/CMakeLists.txt`

```cmake
# src/controller/CMakeLists.txt
add_library(ControllerLib STATIC
    MainController.cpp
)

target_link_libraries(ControllerLib
    PUBLIC
    Qt5::Core
    CoreLib
    GatewayLib
    CommonLib
)

target_include_directories(ControllerLib
    PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)
```

---

## Task 1：定义 Protobuf 协议

### 1.1 编写 `proto/recipe.proto`

```protobuf
// proto/recipe.proto
syntax = "proto3";

package Semiconductor;

// 单个工艺步骤
message ProcessStep {
    string step_name = 1;       // 步骤名称，如 "Etch", "Deposition"
    double temperature = 2;     // 温度 (°C)
    double pressure = 3;        // 压力 (Pa)
    double duration_sec = 4;    // 持续时间 (秒)
    string gas_type = 5;        // 气体类型
    double gas_flow_sccm = 6;   // 气体流量 (sccm)
}

// 配方主体
message Recipe {
    string recipe_id = 1;           // 配方唯一 ID
    string recipe_name = 2;         // 配方名称
    string product_type = 3;        // 产品类型
    string created_at = 4;          // 创建时间 (ISO 8601 字符串)
    string updated_at = 5;          // 更新时间
    repeated ProcessStep steps = 6; // 工艺步骤列表
    map<string, string> metadata = 7; // 扩展键值对 (批次号、操作员等)
}
```

### 1.2 验证生成

CMake 构建后，`protoc` 会自动在 `cmake-build-*/proto/` 下生成：

```
recipe.pb.h    // C++ 头文件
recipe.pb.cc   // C++ 实现
```

验证方式 — 在 CLion 中 Build 项目后，检查生成文件是否存在：

```
cmake-build-debug-visual-studio/proto/recipe.pb.h
cmake-build-debug-visual-studio/proto/recipe.pb.cc
```

### 1.3 关键 API 速查

生成的 C++ 类用法：

```cpp
#include "recipe.pb.h"

// --- 构造 ---
Semiconductor::Recipe recipe;
recipe.set_recipe_id("RCP-001");
recipe.set_recipe_name("Standard Etch");
recipe.set_product_type("Wafer-12inch");
recipe.set_created_at("2026-05-30T10:00:00Z");

// 添加步骤
auto* step = recipe.add_steps();
step->set_step_name("Pre-Clean");
step->set_temperature(25.0);
step->set_pressure(101325.0);
step->set_duration_sec(60.0);
step->set_gas_type("N2");
step->set_gas_flow_sccm(500.0);

// 添加 metadata
(*recipe.mutable_metadata())["operator"] = "Zhang San";
(*recipe.mutable_metadata())["batch"] = "LOT-20260530";

// --- 序列化 ---
std::string binary;
recipe.SerializeToString(&binary);  // 序列化为二进制字符串
QByteArray qba = QByteArray::fromStdString(binary);

// --- 反序列化 ---
Semiconductor::Recipe parsed;
parsed.ParseFromString(binary);
// 或用 QByteArray:
parsed.ParseFromArray(qba.constData(), qba.size());

// --- 读取字段 ---
std::cout << parsed.recipe_id() << std::endl;       // "RCP-001"
std::cout << parsed.steps_size() << std::endl;       // 1
std::cout << parsed.steps(0).step_name() << std::endl; // "Pre-Clean"

for (const auto& s : parsed.steps()) {
    std::cout << s.step_name() << " @ " << s.temperature() << "°C" << std::endl;
}
```

---

## Task 2：实现 Gateway 层

### 2.1 `src/gateway/Protocol.h` — 自定义二进制帧头

```cpp
// src/gateway/Protocol.h
#pragma once
#include <cstdint>

// 🌟 关闭内存对齐，确保帧头严格 8 字节
#pragma pack(push, 1)

struct FrameHeader {
    uint32_t magic;       // 魔数：0x53454D49 (ASCII "SEMI")
    uint32_t payloadLen;  // Payload 长度 (字节)
};

#pragma pack(pop)

// 常量定义
static constexpr uint32_t FRAME_MAGIC = 0x53454D49;
static constexpr size_t   HEADER_SIZE = sizeof(FrameHeader); // = 8

// 🌟 辅助宏：网络字节序转换 (大端)
// Qt 提供 qToBigEndian / qFromBigEndian，也可直接用 ntohl / htonl
// 但如果你的通信双方都是 x86 小端机器，也可以约定直接用小端，省去转换
```

### 2.2 `src/gateway/TcpWorker.h` — 工作线程头文件

```cpp
// src/gateway/TcpWorker.h
#pragma once
#include <QObject>
#include <QTcpSocket>
#include <QByteArray>

class TcpWorker : public QObject {
    Q_OBJECT
public:
    explicit TcpWorker(qintptr socketDescriptor, QObject *parent = nullptr);

signals:
    // 🌟 解析出完整 Protobuf 数据后发出此信号
    void recipeDataReady(const QByteArray &protobufData);
    // 连接断开
    void disconnected();

public slots:
    void start();          // 初始化 socket (在子线程中调用)
    void onReadyRead();    // 数据到达时的槽
    void onDisconnected(); // 连接断开时的槽

private:
    void processBuffer();  // 核心：粘包/半包处理

    qintptr     m_socketDescriptor;
    QTcpSocket* m_socket = nullptr;
    QByteArray  m_buffer;  // 接收缓冲区
};
```

### 2.3 `src/gateway/TcpWorker.cpp` — 粘包/半包核心逻辑

```cpp
// src/gateway/TcpWorker.cpp
#include "TcpWorker.h"
#include "Protocol.h"
#include "common/Logger.h"

TcpWorker::TcpWorker(qintptr socketDescriptor, QObject *parent)
    : QObject(parent), m_socketDescriptor(socketDescriptor)
{
}

void TcpWorker::start() {
    m_socket = new QTcpSocket();

    // 用传入的 descriptor 绑定已有连接
    if (!m_socket->setSocketDescriptor(m_socketDescriptor)) {
        LOG_ERROR("TcpWorker: Failed to set socket descriptor!");
        delete m_socket;
        m_socket = nullptr;
        return;
    }

    connect(m_socket, &QTcpSocket::readyRead, this, &TcpWorker::onReadyRead);
    connect(m_socket, &QTcpSocket::disconnected, this, &TcpWorker::onDisconnected);

    LOG_INFO("TcpWorker: New client connected, fd={}", static_cast<int>(m_socketDescriptor));
}

void TcpWorker::onReadyRead() {
    // 🌟 把所有新数据追加到缓冲区
    m_buffer.append(m_socket->readAll());
    // 然后尝试解析
    processBuffer();
}

void TcpWorker::processBuffer() {
    // 🌟 核心循环：只要缓冲区里还有足够数据，就不断提取完整帧
    while (static_cast<size_t>(m_buffer.size()) >= HEADER_SIZE) {

        // 1. 读取帧头 (不移出缓冲区，先 peek)
        FrameHeader header;
        memcpy(&header, m_buffer.constData(), HEADER_SIZE);

        // 2. 校验魔数
        if (header.magic != FRAME_MAGIC) {
            LOG_WARN("TcpWorker: Invalid magic 0x{:08X}, discarding 1 byte", header.magic);
            m_buffer.remove(0, 1); // 丢弃一个字节，重新对齐
            continue;
        }

        // 3. 计算完整帧所需长度
        size_t totalFrameSize = HEADER_SIZE + header.payloadLen;

        // 4. 检查数据是否完整 (半包情况)
        if (static_cast<size_t>(m_buffer.size()) < totalFrameSize) {
            LOG_DEBUG("TcpWorker: Incomplete frame, need {} bytes, have {} bytes",
                      totalFrameSize, m_buffer.size());
            break; // 等待更多数据
        }

        // 5. 提取 Payload (跳过 8 字节帧头)
        QByteArray payload = m_buffer.mid(HEADER_SIZE, header.payloadLen);
        LOG_INFO("TcpWorker: Complete frame received, payload size={}", payload.size());

        // 6. 发出信号，把纯净的 Protobuf 数据传给上层
        emit recipeDataReady(payload);

        // 7. 从缓冲区移除已处理的帧
        m_buffer.remove(0, static_cast<int>(totalFrameSize));
    }
}

void TcpWorker::onDisconnected() {
    LOG_INFO("TcpWorker: Client disconnected, fd={}", static_cast<int>(m_socketDescriptor));
    if (m_socket) {
        m_socket->deleteLater();
        m_socket = nullptr;
    }
    emit disconnected();
}
```

### 2.4 `include/gateway/MesGateway.h` — 公共头文件 (对外暴露)

```cpp
// include/gateway/MesGateway.h
#pragma once
#include <QTcpServer>

class TcpWorker;

class MesGateway : public QTcpServer {
    Q_OBJECT
public:
    explicit MesGateway(QObject *parent = nullptr);

    // 启动 TCP 服务器
    bool startServer(quint16 port);
    // 停止服务器
    void stopServer();

signals:
    // 🌟 收到完整的 Protobuf 二进制数据后发出
    void recipeRequested(const QByteArray &protobufData);

protected:
    // 🌟 重写：每当有新 TCP 连接进来时触发
    void incomingConnection(qintptr socketDescriptor) override;

private:
    QList<TcpWorker*> m_workers; // 管理所有活跃的 worker
};
```

### 2.5 `src/gateway/MesGateway.cpp` — 线程调度实现

```cpp
// src/gateway/MesGateway.cpp
#include "gateway/MesGateway.h"  // 公共头文件在 include/ 下
#include "TcpWorker.h"           // 私有头文件在同目录 src/gateway/ 下
#include "common/Logger.h"

#include <QThread>

MesGateway::MesGateway(QObject *parent) : QTcpServer(parent) {
}

bool MesGateway::startServer(quint16 port) {
    if (!listen(QHostAddress::Any, port)) {
        LOG_ERROR("MesGateway: Failed to listen on port {}", port);
        return false;
    }
    LOG_INFO("MesGateway: TCP server listening on port {}", port);
    return true;
}

void MesGateway::stopServer() {
    close(); // 停止监听
    // 清理所有 worker
    for (auto* worker : m_workers) {
        worker->deleteLater();
    }
    m_workers.clear();
}

void MesGateway::incomingConnection(qintptr socketDescriptor) {
    LOG_INFO("MesGateway: Incoming connection, fd={}", static_cast<int>(socketDescriptor));

    // 1. 创建 Worker (不能设 parent，因为要 moveToThread)
    auto* worker = new TcpWorker(socketDescriptor);
    m_workers.append(worker);

    // 2. 创建独立线程
    auto* thread = new QThread(this);

    // 3. 把 Worker 移入子线程
    worker->moveToThread(thread);

    // 4. 连接信号和槽

    // 线程启动 → Worker 初始化 socket
    connect(thread, &QThread::started, worker, &TcpWorker::start);

    // Worker 解析出完整数据 → MesGateway 转发给上层
    connect(worker, &TcpWorker::recipeDataReady, this, &MesGateway::recipeRequested);

    // Worker 断开连接 → 清理
    connect(worker, &TcpWorker::disconnected, thread, &QThread::quit);
    connect(worker, &TcpWorker::disconnected, worker, &TcpWorker::deleteLater);
    connect(thread, &QThread::finished, thread, &QThread::deleteLater);

    // 5. 启动线程
    thread->start();
}
```

### 2.6 `src/gateway/CMakeLists.txt` — 补充 ProtoLib 依赖

现有的 `src/gateway/CMakeLists.txt` 已经基本正确，但需要确保链接 `ProtoLib`（如果后续 TcpWorker 需要直接操作 protobuf 数据的话）。目前保持原样即可：

```cmake
# src/gateway/CMakeLists.txt — 当前已正确，无需修改
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

target_include_directories(GatewayLib
    PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)
```

---

## Task 3：实现 Core 层 (RecipeManager)

### 3.1 `include/core/RecipeManager.h` — 公共头文件

```cpp
// include/core/RecipeManager.h
#pragma once
#include <QObject>
#include <QByteArray>

// 前向声明，避免暴露 protobuf 生成的头文件
namespace Semiconductor {
    class Recipe;
}

class RecipeManager : public QObject {
    Q_OBJECT
public:
    explicit RecipeManager(QObject *parent = nullptr);
    ~RecipeManager() override;

    // 从 TCP 传来的二进制数据中反序列化配方
    bool loadRecipeFromData(const QByteArray &data);

    // 将当前内存中的配方序列化为二进制
    QByteArray getSerializedRecipe() const;

    // 获取当前配方 ID
    QString currentRecipeId() const;

    // 获取当前配方名称
    QString currentRecipeName() const;

    // 获取步骤数量
    int stepCount() const;

signals:
    // 配方加载成功后通知 UI 或其他模块
    void recipeLoaded(const QString &recipeId, const QString &recipeName);

private:
    Semiconductor::Recipe* m_currentRecipe; // 🌟 用指针，避免暴露 protobuf 头文件
};
```

### 3.2 `src/core/RecipeManager.cpp` — 实现

```cpp
// src/core/RecipeManager.cpp
#include "core/RecipeManager.h"
#include "recipe.pb.h"   // protoc 生成的头文件
#include "common/Logger.h"

RecipeManager::RecipeManager(QObject *parent)
    : QObject(parent)
    , m_currentRecipe(new Semiconductor::Recipe())
{
}

RecipeManager::~RecipeManager() {
    delete m_currentRecipe;
}

bool RecipeManager::loadRecipeFromData(const QByteArray &data) {
    // 🌟 反序列化
    Semiconductor::Recipe tempRecipe;

    if (!tempRecipe.ParseFromArray(data.constData(), data.size())) {
        LOG_ERROR("RecipeManager: Protobuf parse failed! Data size={} bytes", data.size());
        return false;
    }

    // 基本校验
    if (tempRecipe.recipe_id().empty()) {
        LOG_WARN("RecipeManager: Recipe has no ID, ignoring.");
        return false;
    }

    // 替换当前配方
    m_currentRecipe->CopyFrom(tempRecipe);

    LOG_INFO("RecipeManager: Loaded recipe [{}] '{}' with {} steps",
             m_currentRecipe->recipe_id(),
             m_currentRecipe->recipe_name(),
             m_currentRecipe->steps_size());

    // 打印每个步骤 (调试用)
    for (int i = 0; i < m_currentRecipe->steps_size(); ++i) {
        const auto& step = m_currentRecipe->steps(i);
        LOG_DEBUG("  Step {}: {} | T={}°C | P={}Pa | t={}s | gas={}@{}sccm",
                  i + 1,
                  step.step_name(),
                  step.temperature(),
                  step.pressure(),
                  step.duration_sec(),
                  step.gas_type(),
                  step.gas_flow_sccm());
    }

    // 通知上层
    emit recipeLoaded(
        QString::fromStdString(m_currentRecipe->recipe_id()),
        QString::fromStdString(m_currentRecipe->recipe_name())
    );

    return true;
}

QByteArray RecipeManager::getSerializedRecipe() const {
    std::string binary;
    if (!m_currentRecipe->SerializeToString(&binary)) {
        LOG_ERROR("RecipeManager: Serialization failed!");
        return {};
    }
    return QByteArray::fromStdString(binary);
}

QString RecipeManager::currentRecipeId() const {
    return QString::fromStdString(m_currentRecipe->recipe_id());
}

QString RecipeManager::currentRecipeName() const {
    return QString::fromStdString(m_currentRecipe->recipe_name());
}

int RecipeManager::stepCount() const {
    return m_currentRecipe->steps_size();
}
```

---

## 数据流全景图

完成 Task 1/2/3 后，数据链路如下：

```
[TCP 客户端]
    │
    │  发送: [FrameHeader(8B)] + [Protobuf binary(N B)]
    ▼
[MesGateway::incomingConnection]
    │  创建 TcpWorker，moveToThread
    ▼
[TcpWorker::onReadyRead]          (子线程)
    │  m_buffer.append(readAll())
    │  processBuffer() 循环:
    │    ├─ 校验 magic (0x53454D49)
    │    ├─ 检查 payloadLen 是否到齐
    │    ├─ 提取 payload → emit recipeDataReady(payload)
    │    └─ 移除已处理字节
    ▼
[MesGateway::recipeRequested]     (Qt 跨线程 QueuedConnection)
    │  信号转发
    ▼
[MainController::onRecipeRequested] (主线程)
    │  调用 recipeMgr->loadRecipeFromData(data)
    ▼
[RecipeManager::loadRecipeFromData]
    │  Protobuf 反序列化
    │  校验 + 存储
    │  emit recipeLoaded(id, name)
    ▼
[日志输出 / 未来的 QML 前端]
```

---

## 构建检查清单

完成所有代码后，按此顺序验证：

- [ ] `proto/recipe.proto` 已编写，CMake 构建后 `recipe.pb.h` 生成成功
- [ ] `src/gateway/Protocol.h` 帧头结构体 `sizeof == 8`
- [ ] `TcpWorker::processBuffer()` 能正确处理：
  - 正常单包
  - 粘包 (多个帧一次到达)
  - 半包 (一个帧分多次到达)
- [ ] `MesGateway::incomingConnection` 正确 `moveToThread`
- [ ] `RecipeManager::loadRecipeFromData` 能反序列化并打印日志
- [ ] `MainController` 信号槽连接无报错
- [ ] `src/CMakeLists.txt` 所有子模块已取消注释
- [ ] `src/app/CMakeLists.txt` 链接了所有模块库
- [ ] 运行时 PATH 包含 vcpkg 的 bin 目录 (DLL 依赖)

---

## 常见踩坑点

### 1. `recipe.pb.h` 找不到
`proto/CMakeLists.txt` 中 `target_include_directories` 指向的是 `CMAKE_CURRENT_BINARY_DIR`，`CoreLib` 链接了 `ProtoLib` 后会继承这个路径，所以 `#include "recipe.pb.h"` 即可，不需要加路径前缀。

### 2. moveToThread 后信号槽不生效
`TcpWorker` 在 `moveToThread` 前**不能设 parent**（即构造时 parent 传 `nullptr`）。Qt 不允许把有 parent 的 QObject 移到另一个线程。

### 3. `#pragma pack(1)` 忘记 pop
```cpp
#pragma pack(push, 1)
struct FrameHeader { ... };
#pragma pack(pop)  // ← 必须恢复，否则后续所有结构体都不对齐
```

### 4. Protobuf 链接报错 `undefined reference to absl::...`
根 `CMakeLists.txt` 中必须按 `utf8_range → absl → protobuf` 顺序 `find_package`。你的项目已经正确配置了这一点。

### 5. 运行时找不到 `libprotobuf.dll`
CLion 的 Run Configuration 中需要追加环境变量：
```
PATH=D:\tool\vcpkg\installed\x64-windows\bin;%PATH%
```
