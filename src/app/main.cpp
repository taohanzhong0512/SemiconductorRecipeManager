#include <QCoreApplication>

// 🌟 修改这里：加上模块文件夹前缀
#include "common/Logger.h"
#include "common/ConfigManager.h"
#include "core/RecipeManager.h"
#include "gateway/MesGateway.h"
#include "controller/MainController.h"


int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    // 1. 初始化基础设施
    Logger::init("logs");
    qInstallMessageHandler(Logger::qtMessageHandler);
    LOG_INFO("=== Semiconductor Equipment Control System Starting ===");

    auto* config = ConfigManager::instance();
    config->load("config/config.toml");

    // 2. 实例化各层模块
    RecipeManager recipeMgr;
    MesGateway gateway;

    // 3. 注入依赖，创建控制器
    MainController controller(&recipeMgr, &gateway);

    // 4. 启动 TCP 网关 (从配置读取端口，默认 8888)
    quint16 port = static_cast<quint16>(config->get<int>("Network.Port", 8888));
    if (gateway.startServer(port)) {
        LOG_INFO("System ready. Waiting for MES connection on port {}...", port);
    }

    return app.exec(); // 🌟 必须进入事件循环，否则 TCP Server 无法工作
}
