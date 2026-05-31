#pragma once
#include <QObject>

class RecipeManager;
class MesGateway;

class MainController : public QObject {
    Q_OBJECT
public:
    explicit MainController(RecipeManager* recipeMgr, MesGateway* gateway, QObject *parent = nullptr);

private slots:
    // 接收网关传来的二进制数据，并喂给业务层
    void onRecipeRequested(const QByteArray &protobufData);

private:
    RecipeManager* m_recipeMgr;
    MesGateway* m_gateway;
};
