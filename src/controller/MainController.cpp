#include "controller/MainController.h"
#include "core/RecipeManager.h"
#include "gateway/MesGateway.h"
#include "common/Logger.h"



MainController::MainController(RecipeManager* recipeMgr, MesGateway* gateway, QObject *parent)
    : QObject(parent), m_recipeMgr(recipeMgr), m_gateway(gateway)
{
    // 🌟 核心：通过信号槽将网络层与业务层解耦连接
    // Qt 会自动处理跨线程安全 (QueuedConnection)
    connect(m_gateway, &MesGateway::recipeRequested,
            this, &MainController::onRecipeRequested);
}

void MainController::onRecipeRequested(const QByteArray &protobufData) {
    LOG_INFO("Controller received protobuf data, size: {} bytes", protobufData.size());

    if (m_recipeMgr->loadRecipeFromData(protobufData)) {
        LOG_INFO("Controller: Recipe successfully loaded and parsed!");
    } else {
        LOG_ERROR("Controller: Failed to parse received protobuf data.");
    }
}
