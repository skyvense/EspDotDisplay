#pragma once
#include <Arduino.h>
#include <WebServer.h>
#include "EspSmartWifi.h"

class ConfigWebServer
{
private:
    WebServer *server_;
    EspSmartWifi *wifi_manager_;
    uint16_t port_;
    
    // 网页HTML
    String getIndexPage();
    String getConfigPage();
    String getUpdatePage();
    String getSuccessPage();
    
    // 路由处理函数
    void handleRoot();
    void handleConfig();
    void handleUpdate();
    void handleUpdateUpload();
    void handleSave();
    void handleStatus();
    void handleNotFound();

public:
    ConfigWebServer(EspSmartWifi *wifi_manager, uint16_t port = 80);
    ~ConfigWebServer();
    
    // 初始化并启动服务器
    void begin();
    
    // 处理客户端请求（需要在loop中调用）
    void handleClient();
    
    // 停止服务器
    void stop();
};
