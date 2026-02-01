#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoOTA.h>
#include "EspSmartWifi.h"
#include "DisplayInterface.h"
#include "VfdDisplay.h"
#include "OledDisplay.h"
#include "ConfigWebServer.h"
#include "MqttManager.h"

// ==================== 显示类型选择 ====================
// 修改这里来切换显示类型
// 选项: 1=VFD, 2=OLED
#define DISPLAY_TYPE_VFD  1
#define DISPLAY_TYPE_OLED 2

// 选择显示类型（修改这一行）
// #define DISPLAY_TYPE DISPLAY_TYPE_VFD
#define DISPLAY_TYPE DISPLAY_TYPE_VFD
// ====================================================

// LED引脚定义 (根据你的硬件调整，-1表示不使用LED)
// ESP32-C3-DevKitM-1: 使用 GPIO 2 (避免与 OLED SDA 冲突)
#define LED_PIN 2

// VFD串口引脚定义 (ESP32-C3)
#define VFD_RX_PIN -1        // 不使用RX
#define VFD_TX_PIN 10        // VFD RX 连接到 ESP32 TX (GPIO 10)
#define VFD_BAUD_RATE 9600   // VFD波特率 9600
#define VFD_OFFSET_X 0       // VFD 列偏移（可正可负）
#define VFD_OFFSET_Y 0       // VFD 行偏移（可正可负）

// OLED I2C引脚定义 (ESP32-C3)
// 确认分辨率: 72x40 (0.42寸)
#define OLED_WIDTH 72        // OLED 宽度（像素）
#define OLED_HEIGHT 40       // OLED 高度（像素）
#define OLED_SDA_PIN 5       // I2C SDA 引脚 (实际接线: GPIO 5)
#define OLED_SCL_PIN 6       // I2C SCL 引脚 (实际接线: GPIO 6)
#define OLED_I2C_ADDR 0x3C   // I2C 地址
#define OLED_OFFSET_X 0      // OLED 水平偏移（像素，可正可负）
#define OLED_OFFSET_Y 0      // OLED 垂直偏移（像素，可正可负）

// WiFi管理对象
EspSmartWifi espWifi(LED_PIN);

// 显示对象（根据 DISPLAY_TYPE 选择）
#if DISPLAY_TYPE == DISPLAY_TYPE_VFD
    VfdDisplay displayDevice(&Serial1, VFD_RX_PIN, VFD_TX_PIN, VFD_BAUD_RATE, VFD_OFFSET_X, VFD_OFFSET_Y);
    #define DISPLAY_NAME "VFD"
#elif DISPLAY_TYPE == DISPLAY_TYPE_OLED
    OledDisplay displayDevice(OLED_WIDTH, OLED_HEIGHT, OLED_SDA_PIN, OLED_SCL_PIN, OLED_I2C_ADDR, OLED_OFFSET_X, OLED_OFFSET_Y);
    #define DISPLAY_NAME "OLED"
#else
    #error "Invalid DISPLAY_TYPE! Must be DISPLAY_TYPE_VFD or DISPLAY_TYPE_OLED"
#endif

DisplayInterface* display = &displayDevice;

// Web服务器对象
ConfigWebServer webServer(&espWifi, 80);

// MQTT管理对象
MqttManager mqttManager;

void onMqttMessage(const String& topic, const String& message) {
    if (display && display->isInitialized()) {
        auto sanitizeLine = [](const String& input, int maxLen) -> String {
            String out = "";
            out.reserve(maxLen);
            for (int i = 0; i < input.length() && out.length() < maxLen; i++) {
                char c = input[i];
                if (c == '\r' || c == '\n' || c == '\t') {
                    out += ' ';
                } else if (static_cast<uint8_t>(c) < 0x20) {
                    out += ' ';
                } else {
                    out += c;
                }
            }
            return out;
        };

        // 静态变量：保存上次显示的内容
        static String lastLine1 = "";
        static String lastLine2 = "";
        
        // 处理消息中的换行符
        String displayMessage = message;
        
        // 替换竖线为换行符
        for (int i = 0; i < displayMessage.length(); i++) {
            if (displayMessage[i] == '|') {
                displayMessage.setCharAt(i, '\n');
            }
        }
        
        // 分割为两行（VFD每行20字符）
        const int maxLineLength = 20;
        String line1 = "";
        String line2 = "";
        
        int newlinePos = displayMessage.indexOf('\n');
        if (newlinePos == -1) {
            // 没有换行符，整个消息作为第一行（可能自动换行）
            line1 = displayMessage.substring(0, min(maxLineLength, (int)displayMessage.length()));
            if (displayMessage.length() > maxLineLength) {
                line2 = displayMessage.substring(maxLineLength, min(maxLineLength * 2, (int)displayMessage.length()));
            }
        } else {
            // 有换行符，分别处理两行
            line1 = displayMessage.substring(0, min(newlinePos, maxLineLength));
            if (newlinePos + 1 < displayMessage.length()) {
                line2 = displayMessage.substring(newlinePos + 1, min(newlinePos + 1 + maxLineLength, (int)displayMessage.length()));
            }
        }
        
        // 清理控制字符（避免换行/回车导致光标偏移）
        line1 = sanitizeLine(line1, maxLineLength);
        line2 = sanitizeLine(line2, maxLineLength);

        // 补齐到maxLineLength字符（避免残留旧内容）
        while (line1.length() < maxLineLength) {
            line1 += " ";
        }
        while (line2.length() < maxLineLength) {
            line2 += " ";
        }
        
        // 只更新变化的行
        bool needUpdate = false;
        
        // 检查第一行是否变化
        if (line1 != lastLine1) {
            Serial.println("Line 1 changed, updating...");
            display->setCursor(1, 1);  // VFD坐标从1开始：列1，行1
            delay(10);
            display->print(line1);     // 打印整行
            lastLine1 = line1;
            needUpdate = true;
        }
        
        // 检查第二行是否变化
        if (line2 != lastLine2) {
            Serial.println("Line 2 changed, updating...");
            display->setCursor(1, 2);  // VFD坐标从1开始：列1，行2
            delay(10);
            display->print(line2);     // 打印整行
            lastLine2 = line2;
            needUpdate = true;
        }
        
        if (needUpdate) {
            Serial.println("Message updated on display");
        } else {
            Serial.println("Message unchanged, skipped update");
        }
    }
}

void setup() {
    // 初始化USB串口（用于调试）
    Serial.begin(115200);
    delay(1000);  // 等待串口稳定
    
    Serial.println("\n\n=================================");
    Serial.printf("ESP32-C3 Display System (%s)\n", DISPLAY_NAME);
    Serial.println("with WebServer & MQTT");
    Serial.println("=================================\n");

    // 配置MQTT回调
    mqttManager.setMessageCallback(onMqttMessage);
    
    // 初始化显示屏
    Serial.printf("Initializing %s Display...\n", DISPLAY_NAME);
    if (display->begin()) {
        Serial.printf("%s Display initialized successfully\n", DISPLAY_NAME);
        
        // 显示欢迎信息
        display->clear();
        delay(100);
        display->println("ESP32-C3");
        display->print("Starting...");
        Serial.println("Displayed welcome message on display");
    } else {
        Serial.printf("Failed to initialize %s Display\n", DISPLAY_NAME);
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
        display->clear();
        display->println("AP Mode");
        display->print(WiFi.softAPIP().toString());
        Serial.println("AP Mode - Web interface available at: " + WiFi.softAPIP().toString());
    }
    
    Serial.println("\n=== Setup Complete ===\n");
}

void loop() {
    ArduinoOTA.handle();
    // 处理Web服务器请求
    webServer.handleClient();
    
    // WiFi看门狗 - 监控WiFi连接状态并自动重连
    bool wifiConnected = espWifi.WiFiWatchDog();
    
    static bool mqttConfigured = false;
    static bool mqttConnected = false;  // 与 mqttConfigured 同层，WiFi 断开时需一起重置
    static bool ipDisplayed = false;
    static unsigned long lastUpdate = 0;

    if (wifiConnected) {
        // WiFi已连接
        if (!ipDisplayed) {
            ipDisplayed = true;
            
            display->clear();
            display->println("WiFi OK");
            display->print(WiFi.localIP().toString());
            
            Serial.println("WiFi Status: Connected");
            Serial.print("IP Address: ");
            Serial.println(WiFi.localIP());
            Serial.println("Web interface: http://" + WiFi.localIP().toString());
            Serial.println("IP displayed on display");
            
            delay(2000);  // 显示IP 2秒
        }
        
        // 配置并连接MQTT（仅一次）
        if (!mqttConfigured) {
            Config config = espWifi.getConfig();
            if (config.Server.length() > 0 && config.Topic.length() > 0) {
                Serial.println("Configuring MQTT...");
                display->clear();
                display->print("MQTT...");
                
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
            if (!mqttConnected && mqttManager.isConnected() == false) {
                if (mqttManager.connect()) {
                    mqttConnected = true;
                    display->clear();
                    display->println("MQTT OK");
                    display->print("Listening...");
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
            mqttConnected = false;  // 重置，以便 WiFi 重连后重新连接 MQTT
            display->clear();
            
            if (espWifi.isAPMode()) {
                display->println("AP Mode");
                display->print(WiFi.softAPIP().toString());
            } else {
                display->print("WiFi...");
            }
        }
    }
    
    delay(10);  // 短延迟避免CPU占用过高
}
