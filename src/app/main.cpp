#include <QCoreApplication>
#include <iostream>

// 引入第三方库头文件验证
#include <spdlog/spdlog.h>
#include <toml++/toml.h>

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    // 测试 spdlog
    spdlog::info("Welcome to Semiconductor Recipe Manager!");
    spdlog::warn("This is a warning from spdlog.");

    // 测试 toml++
    auto tbl = toml::parse("config/default_config.toml");
    if (auto port = tbl["Network"]["Port"].value<int>()) {
        spdlog::info("Loaded TOML config - Network Port: {}", *port);
    } else {
        spdlog::error("Failed to read TOML config.");
    }

    return 0; // 暂时不启动事件循环，直接退出
}
