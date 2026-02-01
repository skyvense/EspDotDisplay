#include "ConfigWebServer.h"
#include <Update.h>

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
    server_->on("/update", HTTP_GET, [this]() { handleUpdate(); });
    server_->on("/update", HTTP_POST, [this]() { 
        server_->send(200, "text/plain", (Update.hasError()) ? "Update Failed" : "Update Success! Rebooting...");
        ESP.restart();
    }, [this]() { handleUpdateUpload(); });
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
            background-color: #000;
            color: #ffaa00;
            font-family: 'Courier New', monospace;
            margin: 0;
            padding: 20px;
        }
        .container {
            max-width: 600px;
            margin: 50px auto;
            border: 2px solid #ffaa00;
            padding: 30px;
            box-shadow: 0 0 20px rgba(255, 170, 0, 0.2);
            background: #000;
        }
        h1 {
            text-align: center;
            border-bottom: 2px dashed #ffaa00;
            padding-bottom: 20px;
            margin-bottom: 30px;
            text-transform: uppercase;
            letter-spacing: 2px;
            text-shadow: 0 0 5px #ffaa00;
        }
        .info {
            border: 1px solid #ffaa00;
            padding: 20px;
            margin: 30px 0;
            background: rgba(255, 170, 0, 0.05);
        }
        .info p {
            margin: 15px 0;
            display: flex;
            justify-content: space-between;
            align-items: center;
            border-bottom: 1px dotted #442200;
            padding-bottom: 5px;
        }
        .info p:last-child {
            border-bottom: none;
        }
        .button {
            display: block;
            width: 100%;
            padding: 15px 0;
            margin: 20px 0;
            background: #000;
            color: #ffaa00;
            border: 1px solid #ffaa00;
            text-align: center;
            text-decoration: none;
            text-transform: uppercase;
            font-weight: bold;
            font-size: 16px;
            transition: all 0.3s ease;
            cursor: pointer;
            box-sizing: border-box;
        }
        .button:hover {
            background: #ffaa00;
            color: #000;
            box-shadow: 0 0 15px #ffaa00;
            font-weight: 900;
        }
        .status-val {
            font-weight: bold;
        }
        /* Scanline effect */
        body::before {
            content: " ";
            display: block;
            position: fixed;
            top: 0;
            left: 0;
            bottom: 0;
            right: 0;
            background: linear-gradient(rgba(18, 16, 16, 0) 50%, rgba(0, 0, 0, 0.25) 50%), linear-gradient(90deg, rgba(255, 0, 0, 0.06), rgba(255, 170, 0, 0.02), rgba(0, 0, 255, 0.06));
            z-index: 2;
            background-size: 100% 2px, 3px 100%;
            pointer-events: none;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>ESP32-C3 VFD Display</h1>
        <div class="info">
            <p><span>SYSTEM_STATUS</span> <span id="status" class="status-val">LOADING...</span></p>
            <p><span>WIFI_LINK</span> <span id="wifi" class="status-val">CHECKING...</span></p>
            <p><span>IP_ADDR</span> <span id="ip" class="status-val">---.---.---.---</span></p>
        </div>
        <a href="/config" class="button">[ WIFI & MQTT CONFIG ]</a>
        <a href="/update" class="button">[ FIRMWARE UPGRADE ]</a>
    </div>
    <script>
        function updateStatus() {
            fetch('/status')
                .then(response => response.json())
                .then(data => {
                    document.getElementById('status').textContent = 'ONLINE';
                    document.getElementById('wifi').textContent = data.connected ? 'CONNECTED' : 'DISCONNECTED';
                    document.getElementById('ip').textContent = data.ip || '---.---.---.---';
                })
                .catch(() => {
                    document.getElementById('status').textContent = 'OFFLINE';
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
    <title>WiFi & MQTT CONFIG</title>
    <style>
        body {
            background-color: #000;
            color: #ffaa00;
            font-family: 'Courier New', monospace;
            margin: 0;
            padding: 20px;
        }
        .container {
            max-width: 600px;
            margin: 50px auto;
            border: 2px solid #ffaa00;
            padding: 30px;
            box-shadow: 0 0 20px rgba(255, 170, 0, 0.2);
            background: #000;
        }
        h1 {
            text-align: center;
            border-bottom: 2px dashed #ffaa00;
            padding-bottom: 20px;
            margin-bottom: 30px;
            text-transform: uppercase;
            letter-spacing: 2px;
            text-shadow: 0 0 5px #ffaa00;
        }
        .form-group {
            margin: 25px 0;
        }
        label {
            display: block;
            margin-bottom: 8px;
            color: #ffaa00;
            font-weight: bold;
            text-transform: uppercase;
            font-size: 0.9em;
        }
        label::before {
            content: "> ";
        }
        input {
            width: 100%;
            padding: 12px;
            background: #000;
            color: #ffaa00;
            border: 1px solid #ffaa00;
            font-family: inherit;
            box-sizing: border-box;
            font-size: 16px;
        }
        input:focus {
            outline: none;
            box-shadow: 0 0 10px #ffaa00;
            background: #110800;
        }
        .button {
            display: block;
            width: 100%;
            padding: 15px 0;
            margin: 30px 0 20px 0;
            background: #000;
            color: #ffaa00;
            border: 1px solid #ffaa00;
            text-align: center;
            text-decoration: none;
            text-transform: uppercase;
            font-weight: bold;
            font-size: 16px;
            transition: all 0.3s ease;
            cursor: pointer;
        }
        .button:hover {
            background: #ffaa00;
            color: #000;
            box-shadow: 0 0 15px #ffaa00;
            font-weight: 900;
        }
        .back-link {
            display: block;
            text-align: center;
            margin-top: 20px;
            color: #ffaa00;
            text-decoration: none;
            opacity: 0.7;
            text-transform: uppercase;
            font-size: 0.8em;
        }
        .back-link:hover {
            opacity: 1;
            text-decoration: underline;
        }
        /* Scanline effect */
        body::before {
            content: " ";
            display: block;
            position: fixed;
            top: 0;
            left: 0;
            bottom: 0;
            right: 0;
            background: linear-gradient(rgba(18, 16, 16, 0) 50%, rgba(0, 0, 0, 0.25) 50%), linear-gradient(90deg, rgba(255, 0, 0, 0.06), rgba(255, 170, 0, 0.02), rgba(0, 0, 255, 0.06));
            z-index: 2;
            background-size: 100% 2px, 3px 100%;
            pointer-events: none;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>WIFI & MQTT CONFIG</h1>
        <form method="POST" action="/save">
            <div class="form-group">
                <label>WiFi SSID:</label>
                <input type="text" name="ssid" value=")html" + config.SSID + R"html(" required>
            </div>
            <div class="form-group">
                <label>WiFi PASSWORD:</label>
                <input type="password" name="password" value=")html" + config.Passwd + R"html(">
            </div>
            <div class="form-group">
                <label>MQTT SERVER:</label>
                <input type="text" name="server" value=")html" + config.Server + R"html(" placeholder="mqtt://user:pass@host:port">
            </div>
            <div class="form-group">
                <label>MQTT TOPIC:</label>
                <input type="text" name="topic" value=")html" + config.Topic + R"html(" placeholder="/topic/path">
            </div>
            <button type="submit" class="button">[ SAVE CONFIGURATION ]</button>
        </form>
        <a href="/" class="back-link">&lt; BACK TO MAIN</a>
    </div>
</body>
</html>
)html";
    return html;
}

String ConfigWebServer::getUpdatePage()
{
    String html = R"html(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>FIRMWARE UPGRADE</title>
    <style>
        body {
            background-color: #000;
            color: #ffaa00;
            font-family: 'Courier New', monospace;
            margin: 0;
            padding: 20px;
        }
        .container {
            max-width: 600px;
            margin: 50px auto;
            border: 2px solid #ffaa00;
            padding: 30px;
            box-shadow: 0 0 20px rgba(255, 170, 0, 0.2);
            background: #000;
        }
        h1 {
            text-align: center;
            border-bottom: 2px dashed #ffaa00;
            padding-bottom: 20px;
            margin-bottom: 30px;
            text-transform: uppercase;
            letter-spacing: 2px;
            text-shadow: 0 0 5px #ffaa00;
        }
        .form-group {
            margin: 20px 0;
            text-align: center;
        }
        .button {
            display: block;
            width: 100%;
            padding: 15px 0;
            margin: 30px 0 20px 0;
            background: #000;
            color: #ffaa00;
            border: 1px solid #ffaa00;
            text-align: center;
            text-decoration: none;
            text-transform: uppercase;
            font-weight: bold;
            font-size: 16px;
            transition: all 0.3s ease;
            cursor: pointer;
        }
        .button:hover {
            background: #ffaa00;
            color: #000;
            box-shadow: 0 0 15px #ffaa00;
            font-weight: 900;
        }
        .back-link {
            display: block;
            text-align: center;
            margin-top: 20px;
            color: #ffaa00;
            text-decoration: none;
            opacity: 0.7;
            text-transform: uppercase;
            font-size: 0.8em;
        }
        .back-link:hover {
            opacity: 1;
            text-decoration: underline;
        }
        #progress-bar {
            width: 100%;
            border: 2px solid #ffaa00;
            margin-top: 20px;
            height: 30px;
            display: none;
            position: relative;
        }
        #progress {
            width: 0%;
            height: 100%;
            background-color: #ffaa00;
            text-align: center;
            line-height: 30px;
            color: #000;
            font-weight: bold;
        }
        input[type="file"] {
            border: 1px dashed #ffaa00;
            padding: 20px;
            width: 100%;
            box-sizing: border-box;
            background: rgba(255, 170, 0, 0.05);
            color: #ffaa00;
        }
        /* Scanline effect */
        body::before {
            content: " ";
            display: block;
            position: fixed;
            top: 0;
            left: 0;
            bottom: 0;
            right: 0;
            background: linear-gradient(rgba(18, 16, 16, 0) 50%, rgba(0, 0, 0, 0.25) 50%), linear-gradient(90deg, rgba(255, 0, 0, 0.06), rgba(255, 170, 0, 0.02), rgba(0, 0, 255, 0.06));
            z-index: 2;
            background-size: 100% 2px, 3px 100%;
            pointer-events: none;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>FIRMWARE UPGRADE</h1>
        <form method="POST" action="/update" enctype="multipart/form-data" id="upload_form">
            <div class="form-group">
                <input type="file" name="update" accept=".bin" required>
            </div>
            <button type="submit" class="button">[ START UPGRADE ]</button>
        </form>
        <div id="progress-bar">
            <div id="progress">0%</div>
        </div>
        <a href="/" class="back-link">&lt; BACK TO MAIN</a>
    </div>
    <script>
        // 简单的文件上传进度显示（注：标准表单提交无法直接获取上传进度，这里仅作为占位，
        // 实际使用AJAX上传可实现进度条，为简化起见保持基础表单提交）
        document.getElementById('upload_form').onsubmit = function() {
            document.getElementById('progress-bar').style.display = 'block';
            document.getElementById('progress').style.width = '100%';
            document.getElementById('progress').textContent = 'UPLOADING...';
        };
    </script>
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
    <title>SUCCESS</title>
    <style>
        body {
            background-color: #000;
            color: #ffaa00;
            font-family: 'Courier New', monospace;
            margin: 0;
            padding: 20px;
        }
        .container {
            max-width: 600px;
            margin: 50px auto;
            border: 2px solid #ffaa00;
            padding: 30px;
            box-shadow: 0 0 20px rgba(255, 170, 0, 0.2);
            text-align: center;
            background: #000;
        }
        h1 {
            color: #ffaa00;
            text-transform: uppercase;
            border-bottom: 2px dashed #ffaa00;
            padding-bottom: 20px;
            margin-bottom: 30px;
        }
        p {
            font-size: 16px;
            color: #ffaa00;
            margin: 10px 0;
            text-transform: uppercase;
        }
        /* Scanline effect */
        body::before {
            content: " ";
            display: block;
            position: fixed;
            top: 0;
            left: 0;
            bottom: 0;
            right: 0;
            background: linear-gradient(rgba(18, 16, 16, 0) 50%, rgba(0, 0, 0, 0.25) 50%), linear-gradient(90deg, rgba(255, 0, 0, 0.06), rgba(255, 170, 0, 0.02), rgba(0, 0, 255, 0.06));
            z-index: 2;
            background-size: 100% 2px, 3px 100%;
            pointer-events: none;
        }
        .blink {
            animation: blinker 1s linear infinite;
        }
        @keyframes blinker {
            50% { opacity: 0; }
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>✓ CONFIG SAVED</h1>
        <p>REBOOTING SYSTEM...</p>
        <p>CONNECTING TO WIFI...</p>
        <p class="blink">PLEASE WAIT...</p>
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

void ConfigWebServer::handleUpdate()
{
    server_->send(200, "text/html", getUpdatePage());
}

void ConfigWebServer::handleUpdateUpload()
{
    HTTPUpload& upload = server_->upload();
    if (upload.status == UPLOAD_FILE_START) {
        Serial.printf("Update: %s\n", upload.filename.c_str());
        if (!Update.begin(UPDATE_SIZE_UNKNOWN)) { // start with max available size
            Update.printError(Serial);
        }
    } else if (upload.status == UPLOAD_FILE_WRITE) {
        // Flashing firmware to ESP
        if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
            Update.printError(Serial);
        }
    } else if (upload.status == UPLOAD_FILE_END) {
        if (Update.end(true)) { // true to set the size to the current progress
            Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
        } else {
            Update.printError(Serial);
        }
    }
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
        
        // 先让响应发完再重启：在 2 秒内反复处理连接，避免 TCP 未发完就重启
        for (unsigned long t = millis(); millis() - t < 2000;) {
            delay(10);
            server_->handleClient();
        }
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
