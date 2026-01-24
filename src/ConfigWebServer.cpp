#include "ConfigWebServer.h"

ConfigWebServer::ConfigWebServer(EspSmartWifi *wifi_manager, uint16_t port)
    : wifi_manager_(wifi_manager), port_(port), server_(nullptr)
{
    server_ = new WebServer(port_);
}

ConfigWebServer::~ConfigWebServer()
{
    if (server_) {
        server_->stop();
        delete server_;
    }
}

void ConfigWebServer::begin()
{
    if (!server_) return;
    
    // 设置路由
    server_->on("/", [this]() { handleRoot(); });
    server_->on("/config", [this]() { handleConfig(); });
    server_->on("/save", HTTP_POST, [this]() { handleSave(); });
    server_->on("/status", [this]() { handleStatus(); });
    server_->onNotFound([this]() { handleNotFound(); });
    
    server_->begin();
    Serial.printf("Web server started on port %d\n", port_);
}

void ConfigWebServer::handleClient()
{
    if (server_) {
        server_->handleClient();
    }
}

void ConfigWebServer::stop()
{
    if (server_) {
        server_->stop();
    }
}

String ConfigWebServer::getIndexPage()
{
    String html = R"html(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>ESP32-C3 VFD Display</title>
    <style>
        body {
            font-family: Arial, sans-serif;
            max-width: 600px;
            margin: 50px auto;
            padding: 20px;
            background: #f0f0f0;
        }
        .container {
            background: white;
            padding: 30px;
            border-radius: 10px;
            box-shadow: 0 2px 10px rgba(0,0,0,0.1);
        }
        h1 {
            color: #333;
            text-align: center;
        }
        .info {
            background: #e8f4f8;
            padding: 15px;
            border-radius: 5px;
            margin: 20px 0;
        }
        .button {
            display: block;
            width: 100%;
            padding: 15px;
            margin: 10px 0;
            background: #4CAF50;
            color: white;
            text-align: center;
            text-decoration: none;
            border-radius: 5px;
            border: none;
            font-size: 16px;
            cursor: pointer;
        }
        .button:hover {
            background: #45a049;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>ESP32-C3 VFD Display</h1>
        <div class="info">
            <p><strong>设备状态:</strong> <span id="status">加载中...</span></p>
            <p><strong>WiFi状态:</strong> <span id="wifi">检查中...</span></p>
            <p><strong>IP地址:</strong> <span id="ip">-</span></p>
        </div>
        <a href="/config" class="button">WiFi & MQTT 配置</a>
    </div>
    <script>
        function updateStatus() {
            fetch('/status')
                .then(response => response.json())
                .then(data => {
                    document.getElementById('status').textContent = '在线';
                    document.getElementById('wifi').textContent = data.connected ? '已连接' : '未连接';
                    document.getElementById('ip').textContent = data.ip || '-';
                })
                .catch(() => {
                    document.getElementById('status').textContent = '离线';
                });
        }
        updateStatus();
        setInterval(updateStatus, 5000);
    </script>
</body>
</html>
)html";
    return html;
}

String ConfigWebServer::getConfigPage()
{
    Config config = wifi_manager_->getConfig();
    
    String html = R"html(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>WiFi & MQTT 配置</title>
    <style>
        body {
            font-family: Arial, sans-serif;
            max-width: 600px;
            margin: 50px auto;
            padding: 20px;
            background: #f0f0f0;
        }
        .container {
            background: white;
            padding: 30px;
            border-radius: 10px;
            box-shadow: 0 2px 10px rgba(0,0,0,0.1);
        }
        h1 {
            color: #333;
            text-align: center;
        }
        .form-group {
            margin: 20px 0;
        }
        label {
            display: block;
            margin-bottom: 5px;
            color: #555;
            font-weight: bold;
        }
        input {
            width: 100%;
            padding: 10px;
            border: 1px solid #ddd;
            border-radius: 5px;
            box-sizing: border-box;
        }
        .button {
            display: block;
            width: 100%;
            padding: 15px;
            margin: 20px 0;
            background: #4CAF50;
            color: white;
            text-align: center;
            border-radius: 5px;
            border: none;
            font-size: 16px;
            cursor: pointer;
        }
        .button:hover {
            background: #45a049;
        }
        .back-link {
            display: block;
            text-align: center;
            margin-top: 20px;
            color: #666;
            text-decoration: none;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>WiFi & MQTT 配置</h1>
        <form method="POST" action="/save">
            <div class="form-group">
                <label>WiFi SSID:</label>
                <input type="text" name="ssid" value=")html" + config.SSID + R"html(" required>
            </div>
            <div class="form-group">
                <label>WiFi 密码:</label>
                <input type="password" name="password" value=")html" + config.Passwd + R"html(">
            </div>
            <div class="form-group">
                <label>MQTT 服务器:</label>
                <input type="text" name="server" value=")html" + config.Server + R"html(" placeholder="mqtt://user:pass@host:port">
            </div>
            <div class="form-group">
                <label>MQTT 主题:</label>
                <input type="text" name="topic" value=")html" + config.Topic + R"html(" placeholder="/topic/path">
            </div>
            <button type="submit" class="button">保存配置</button>
        </form>
        <a href="/" class="back-link">返回首页</a>
    </div>
</body>
</html>
)html";
    return html;
}

String ConfigWebServer::getSuccessPage()
{
    String html = R"html(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>配置成功</title>
    <style>
        body {
            font-family: Arial, sans-serif;
            max-width: 600px;
            margin: 50px auto;
            padding: 20px;
            background: #f0f0f0;
        }
        .container {
            background: white;
            padding: 30px;
            border-radius: 10px;
            box-shadow: 0 2px 10px rgba(0,0,0,0.1);
            text-align: center;
        }
        h1 {
            color: #4CAF50;
        }
        p {
            font-size: 16px;
            color: #666;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>✓ 配置保存成功</h1>
        <p>设备正在重启并连接WiFi...</p>
        <p>请稍候片刻后刷新页面</p>
    </div>
    <script>
        setTimeout(() => {
            window.location.href = '/';
        }, 5000);
    </script>
</body>
</html>
)html";
    return html;
}

void ConfigWebServer::handleRoot()
{
    server_->send(200, "text/html", getIndexPage());
}

void ConfigWebServer::handleConfig()
{
    server_->send(200, "text/html", getConfigPage());
}

void ConfigWebServer::handleSave()
{
    if (server_->method() != HTTP_POST) {
        server_->send(405, "text/plain", "Method Not Allowed");
        return;
    }
    
    Config config;
    config.SSID = server_->arg("ssid");
    config.Passwd = server_->arg("password");
    config.Server = server_->arg("server");
    config.Topic = server_->arg("topic");
    config.bConfigValid = true;
    
    Serial.println("Saving new configuration:");
    Serial.printf("  SSID: %s\n", config.SSID.c_str());
    Serial.printf("  Server: %s\n", config.Server.c_str());
    Serial.printf("  Topic: %s\n", config.Topic.c_str());
    
    if (wifi_manager_->SaveConfig(config)) {
        server_->send(200, "text/html", getSuccessPage());
        
        // 延迟后重启以应用新配置
        delay(1000);
        ESP.restart();
    } else {
        server_->send(500, "text/plain", "Failed to save configuration");
    }
}

void ConfigWebServer::handleStatus()
{
    String json = "{";
    json += "\"connected\":" + String(WiFi.status() == WL_CONNECTED ? "true" : "false");
    json += ",\"ip\":\"" + WiFi.localIP().toString() + "\"";
    json += ",\"rssi\":" + String(WiFi.RSSI());
    json += ",\"ap_mode\":" + String(wifi_manager_->isAPMode() ? "true" : "false");
    json += "}";
    
    server_->send(200, "application/json", json);
}

void ConfigWebServer::handleNotFound()
{
    server_->send(404, "text/plain", "Not Found");
}
