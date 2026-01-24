#include <Arduino.h>
#include <ArduinoJson.h>
#include <FS.h>
#include <SPIFFS.h>
#include "EspSmartWifi.h"

void reset() 
{ 
    ESP.restart();
}

// LED闪烁实现
void EspSmartWifi::ledBlink(int times, int onMs, int offMs) {
    if (led_pin_ < 0) return;
    for (int i = 0; i < times; i++) {
        digitalWrite(led_pin_, HIGH);
        delay(onMs);
        digitalWrite(led_pin_, LOW);
        if (i < times - 1) delay(offMs);
    }
}

bool EspSmartWifi::LoadConfig()
{
    Serial.println("\n=== Loading WiFi Configuration ===");
    if (!SPIFFS.exists("/config.json")) {
        Serial.println("Config file not found, using defaults");
        return false;
    }
    Serial.println("Found config.json");

    File configFile = SPIFFS.open("/config.json", "r");
    if (!configFile) {
        Serial.println("Failed to open config file");
        return false;
    }
    Serial.println("Config file opened successfully");

    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, configFile);
    configFile.close();

    if (error) {
        Serial.print("Failed to parse config file: ");
        Serial.println(error.c_str());
        return false;
    }
    Serial.println("JSON parsed successfully");

    _config.SSID = doc["ssid"] | "1";
    _config.Passwd = doc["passwd"] | "1";
    _config.Server = doc["server"] | "192.1.8.3";
    _config.Topic = doc["topic"] | "/espRouterPower/power";
    _config.bConfigValid = true;

    Serial.println("WiFi configuration loaded successfully");
    Serial.println("=== WiFi Configuration Load Complete ===\n");

    return true;
}

bool EspSmartWifi::SaveConfig(Config config)
{
    _config = config;
    return SaveConfig();
}

bool EspSmartWifi::SaveConfig()
{
    Serial.println("\n=== Saving WiFi Configuration ===");
    StaticJsonDocument<512> doc;
    doc["ssid"] = _config.SSID;
    doc["passwd"] = _config.Passwd;
    doc["server"] = _config.Server;
    doc["topic"] = _config.Topic;

    File configFile = SPIFFS.open("/config.json", "w");
    if (!configFile) {
        Serial.println("Failed to open config file for writing");
        return false;
    }

    if (serializeJson(doc, configFile) == 0) {
        Serial.println("Failed to write to config file");
        configFile.close();
        return false;
    }

    configFile.close();
    Serial.println("WiFi configuration saved successfully");
    Serial.println("=== WiFi Configuration Save Complete ===\n");
    return true;
}

bool EspSmartWifi::loadRelayStatesFromFlash() {
    if (!SPIFFS.exists("/relay_states.json")) {
        Serial.println("No relay states file found, using defaults (all ON)");
        // 使用默认值（全部开启）
        for (int i = 0; i < 6; i++) {
            cachedRelayStates[i] = true;
        }
        statesLoaded = true;
        return true;
    }

    File file = SPIFFS.open("/relay_states.json", "r");
    if (!file) {
        Serial.println("Failed to open relay states file");
        return false;
    }

    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, file);
    file.close();

    if (error) {
        Serial.println("Failed to parse relay states file");
        return false;
    }

    // 更新缓存, 默认值为ON
    for (int i = 0; i < 6; i++) {
        cachedRelayStates[i] = doc["relay" + String(i)] | true;
    }
    statesLoaded = true;
    return true;
}

bool EspSmartWifi::saveRelayStatesToFlash() {
    StaticJsonDocument<256> doc;
    for (int i = 0; i < 6; i++) {
        doc["relay" + String(i)] = cachedRelayStates[i];
    }

    File file = SPIFFS.open("/relay_states.json", "w");
    if (!file) {
        Serial.println("Failed to open relay states file for writing");
        return false;
    }

    if (serializeJson(doc, file) == 0) {
        Serial.println("Failed to write relay states");
        file.close();
        return false;
    }

    file.close();
    return true;
}

bool EspSmartWifi::updateRelayState(int index, bool state) {
    if (index >= 0 && index < 6) {
        cachedRelayStates[index] = state;  // 更新缓存
        statesLoaded = true;  // 标记已加载
        return saveRelayStatesToFlash();  // 保存到flash
    }
    return false;
}

bool EspSmartWifi::getRelayStates(bool states[6]) {
    if (!statesLoaded) {
        // 如果缓存未加载，从flash读取
        if (!loadRelayStatesFromFlash()) {
            return false;
        }
    }
    // 从缓存复制状态
    memcpy(states, cachedRelayStates, sizeof(bool) * 6);
    return true;
}

void EspSmartWifi::syncRelayStates() {
    Serial.println("\n=== Syncing Relay States to Hardware ===");
    
    // 确保状态已加载
    if (!statesLoaded) {
        if (!loadRelayStatesFromFlash()) {
            Serial.println("Failed to load relay states");
            return;
        }
    }
}

void EspSmartWifi::BaseConfig()
{
    // 尝试连接WiFi
  WiFi.mode(WIFI_STA);    
  WiFi.begin(_config.SSID.c_str(), _config.Passwd.c_str());
    
    // 等待连接，最多等待10秒
    int waitCount = 0;
    while (WiFi.status() != WL_CONNECTED && waitCount < 20) {
        delay(500);
        Serial.print(".");
        waitCount++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nWiFi connected");
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());
        ledBlink(3, 100, 100);  // 快速闪烁3次表示连接成功
    } else {
        Serial.println("\nWiFi connection failed, will retry...");
        ledBlink(1, 1000, 1000);  // 慢闪表示等待重连
    }
}

void EspSmartWifi::StartAPMode()
{
    if (_isAPMode) {
        Serial_debug.println("Already in AP mode, skipping initialization");
        return;
    }

    Serial_debug.println("Starting AP mode...");
    
    // 创建唯一的AP名称 (ESP32-C3使用MAC地址)
    uint64_t chipid = ESP.getEfuseMac();
    String apName = "ESP_Config_" + String((uint32_t)(chipid >> 32), HEX) + String((uint32_t)chipid, HEX);
    
    // 配置AP模式
    WiFi.mode(WIFI_AP);
    WiFi.softAP(apName.c_str(), "12345678", 6);  // 使用固定的密码
    
    Serial_debug.print("AP started with SSID: ");
    Serial_debug.println(apName);
    Serial_debug.print("AP IP address: ");
    Serial_debug.println(WiFi.softAPIP());
    
    _isAPMode = true;
    ledBlink(2, 100, 100);  // 慢闪表示AP模式
}

void EspSmartWifi::StopAPMode() {
    if (!_isAPMode) return;
    Serial_debug.println("Stopping AP mode, switching to STA mode...");
    WiFi.softAPdisconnect(true);
    WiFi.mode(WIFI_STA);
    _isAPMode = false;
    ledBlink(1, 100, 100); // 快闪表示退出AP
}

void EspSmartWifi::TryConnectWifi() {
    Serial_debug.println("Trying to connect to WiFi...");
    Serial_debug.print("SSID: ");
    Serial_debug.println(_config.SSID);
    Serial_debug.print("Password: ");
    Serial_debug.println(_config.Passwd);
    
    WiFi.mode(WIFI_STA);
    WiFi.begin(_config.SSID.c_str(), _config.Passwd.c_str());
}

void EspSmartWifi::ConnectWifi()
{
    // 加载配置
    if (!LoadConfig()) {
        Serial.println("No WiFi configuration found, entering AP mode...");
        StartAPMode();
        return;
    }
    
    // 尝试连接WiFi
    BaseConfig();
}

bool EspSmartWifi::WiFiWatchDog()
{
    static unsigned long lastCheck = 0;
    static unsigned long lastModeSwitchMillis = 0;
    static bool apModeActive = false;
    unsigned long now = millis();

    // 每5秒检查一次WiFi状态
    if (now - lastCheck < 5000) {
        return WiFi.status() == WL_CONNECTED;
    }
    lastCheck = now;

    if (WiFi.status() != WL_CONNECTED) {        
        if (!_isAPMode && _config.bConfigValid)
        {
            Serial.println("WiFi disconnected, attempting to reconnect...");
            ledBlink(1, 100, 100);  // 慢闪表示等待重连
            WiFi.begin(_config.SSID.c_str(), _config.Passwd.c_str());
        }  

        // 自动AP/STA切换逻辑
        if (!_isAPMode && now - lastModeSwitchMillis > 60000) {  // 1分钟后进入AP模式
            StartAPMode();
            apModeActive = true;
            lastModeSwitchMillis = now;
        } else if (_isAPMode && apModeActive && now - lastModeSwitchMillis > 180000) {  // 3分钟后退出AP模式
            StopAPMode();
            TryConnectWifi();
            apModeActive = false;
            lastModeSwitchMillis = now;
        }
    } else {
        // WiFi已连接
        if (_isAPMode) {
            StopAPMode();
        }
        apModeActive = false;
        lastModeSwitchMillis = now;
    }

    return WiFi.status() == WL_CONNECTED;
}

void EspSmartWifi::initFS()
{  
    Serial_debug.println("\nMounting SPIFFS...");
    if (!SPIFFS.begin(true)) {  // true = 格式化失败时自动格式化
        Serial_debug.println("Failed to mount SPIFFS");
        return;
    }
    
    // 列出文件系统中的所有文件
    Serial_debug.println("\nFiles in SPIFFS:");
    File root = SPIFFS.open("/");
    File file = root.openNextFile();
    bool hasFiles = false;
    while (file) {
        hasFiles = true;
        Serial_debug.print("  ");
        Serial_debug.print(file.name());
        Serial_debug.print("  ");
        Serial_debug.print(file.size());
        Serial_debug.println(" bytes");
        file = root.openNextFile();
    }
    if (!hasFiles) {
        Serial_debug.println("  No files found");
    }
    
    Serial_debug.println("\nSPIFFS mounted successfully");
    Serial_debug.println("=== SPIFFS Initialization Complete ===\n");
}

void EspSmartWifi::DisplayIP()
{

}

String EspSmartWifi::httpGet(const String& path) {
    if (WiFi.status() != WL_CONNECTED) {
        Serial_debug.println("WiFi not connected");
        return "";
    }

    String url = _config.Server + path + "?icon=https://support.arduino.cc/hc/article_attachments/12416033021852.png";
    Serial_debug.print("HTTP GET: ");
    Serial_debug.println(url);

    HTTPClient http;
    http.begin(client, url);
    int httpCode = http.GET();

    String payload = "";
    if (httpCode > 0) {
        if (httpCode == HTTP_CODE_OK) {
            payload = http.getString();
            Serial_debug.println("HTTP Response: " + payload);
        } else {
            Serial_debug.printf("HTTP GET failed, error: %s\n", http.errorToString(httpCode).c_str());
        }
    } else {
        Serial_debug.printf("HTTP GET failed, error: %s\n", http.errorToString(httpCode).c_str());
    }

    http.end();
    return payload;
}