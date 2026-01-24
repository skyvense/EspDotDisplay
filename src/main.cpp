#include <Arduino.h>
#include "EspSmartWifi.h"
#include "VfdDisplay.h"
#include "ConfigWebServer.h"
#include "MqttManager.h"

// LED引脚定义 (根据你的硬件调整，-1表示不使用LED)
// ESP32-C3-DevKitM-1 内置LED通常在GPIO8
#define LED_PIN 8

// VFD串口引脚定义 (ESP32-C3)
// 控制信号接在GPIO 10 (TX)，接收可选
#define VFD_RX_PIN -1        // 不使用RX（如果VFD只需要接收数据）
#define VFD_TX_PIN 10        // VFD RX 连接到 ESP32 TX (GPIO 10)
#define VFD_BAUD_RATE 9600   // VFD波特率 9600

// WiFi管理对象
EspSmartWifi espWifi(LED_PIN);

// VFD显示屏对象
VfdDisplay vfd(&Serial1, VFD_RX_PIN, VFD_TX_PIN, VFD_BAUD_RATE);

// Web服务器对象
ConfigWebServer webServer(&espWifi, 80);

// MQTT管理对象
MqttManager mqttManager(&vfd);

void setup() {
    // 初始化USB串口（用于调试）
    Serial.begin(115200);
    delay(1000);  // 等待串口稳定
    
    Serial.println("\n\n=================================");
    Serial.println("ESP32-C3 VFD Display System");
    Serial.println("with WebServer & MQTT");
    Serial.println("=================================\n");
    
    // 初始化VFD显示屏
    Serial.println("Initializing VFD Display...");
    if (vfd.begin()) {
        Serial.println("VFD Display initialized successfully");
        
        // 显示欢迎信息
        vfd.clear();
        delay(100);
        vfd.println("ESP32-C3");
        vfd.print("Starting...");
        Serial.println("Displayed welcome message on VFD");
    } else {
        Serial.println("Failed to initialize VFD Display");
    }
    
    // 初始化文件系统
    espWifi.initFS();
    
    // 连接WiFi
    espWifi.ConnectWifi();
    
    // 启动Web服务器
    webServer.begin();
    Serial.println("Web server started");
    
    // 显示访问信息
    if (espWifi.isAPMode()) {
        vfd.clear();
        vfd.println("AP Mode");
        vfd.print(WiFi.softAPIP().toString());
        Serial.println("AP Mode - Web interface available at: " + WiFi.softAPIP().toString());
    }
    
    Serial.println("\n=== Setup Complete ===\n");
}

void loop() {
    // 处理Web服务器请求
    webServer.handleClient();
    
    // WiFi看门狗 - 监控WiFi连接状态并自动重连
    bool wifiConnected = espWifi.WiFiWatchDog();
    
    static bool mqttConfigured = false;
    static bool ipDisplayed = false;
    static unsigned long lastUpdate = 0;
    
    if (wifiConnected) {
        // WiFi已连接
        if (!ipDisplayed) {
            ipDisplayed = true;
            
            vfd.clear();
            vfd.println("WiFi OK");
            vfd.print(WiFi.localIP().toString());
            
            Serial.println("WiFi Status: Connected");
            Serial.print("IP Address: ");
            Serial.println(WiFi.localIP());
            Serial.println("Web interface: http://" + WiFi.localIP().toString());
            Serial.println("IP displayed on VFD");
            
            delay(2000);  // 显示IP 2秒
        }
        
        // 配置并连接MQTT（仅一次）
        if (!mqttConfigured) {
            Config config = espWifi.getConfig();
            if (config.Server.length() > 0 && config.Topic.length() > 0) {
                Serial.println("Configuring MQTT...");
                vfd.clear();
                vfd.print("MQTT...");
                
                if (mqttManager.configure(config.Server, config.Topic)) {
                    mqttConfigured = true;
                    Serial.println("MQTT configured successfully");
                } else {
                    Serial.println("MQTT configuration failed");
                }
            } else {
                Serial.println("MQTT not configured in settings");
            }
        }
        
        // 处理MQTT连接和消息
        if (mqttConfigured) {
            // 首次连接
            static bool mqttConnected = false;
            if (!mqttConnected && mqttManager.isConnected() == false) {
                if (mqttManager.connect()) {
                    mqttConnected = true;
                    vfd.clear();
                    vfd.println("MQTT OK");
                    vfd.print("Listening...");
                    Serial.println("MQTT connected and subscribed");
                }
            }
            
            // 保持MQTT循环
            mqttManager.loop();
            
            // 检查MQTT连接状态
            if (mqttConnected && !mqttManager.isConnected()) {
                mqttConnected = false;
                Serial.println("MQTT disconnected");
            }
        }
        
        // 定期打印状态信息
        if (millis() - lastUpdate > 30000) {
            lastUpdate = millis();
            Serial.println("=== Status Update ===");
            Serial.println("WiFi: Connected");
            Serial.print("IP: ");
            Serial.println(WiFi.localIP());
            Serial.print("RSSI: ");
            Serial.print(WiFi.RSSI());
            Serial.println(" dBm");
            Serial.print("MQTT: ");
            Serial.println(mqttManager.isConnected() ? "Connected" : "Disconnected");
            Serial.println("=====================");
        }
        
    } else {
        // WiFi未连接
        if (ipDisplayed) {
            ipDisplayed = false;
            mqttConfigured = false;
            vfd.clear();
            
            if (espWifi.isAPMode()) {
                vfd.println("AP Mode");
                vfd.print(WiFi.softAPIP().toString());
            } else {
                vfd.print("WiFi...");
            }
        }
    }
    
    delay(10);  // 短延迟避免CPU占用过高
}
