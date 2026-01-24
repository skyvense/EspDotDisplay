#include "MqttManager.h"

// 静态成员初始化
MqttManager* MqttManager::instance_ = nullptr;

MqttManager::MqttManager(VfdDisplay *vfd)
    : vfd_(vfd), connected_(false), last_reconnect_attempt_(0),
      mqtt_port_(1883)
{
    wifi_client_ = new WiFiClient();
    mqtt_client_ = new PubSubClient(*wifi_client_);
    mqtt_client_->setCallback(messageCallback);
    instance_ = this;
}

MqttManager::~MqttManager()
{
    disconnect();
    if (mqtt_client_) delete mqtt_client_;
    if (wifi_client_) delete wifi_client_;
    instance_ = nullptr;
}

bool MqttManager::parseMqttUrl(const String& url)
{
    // 解析格式: mqtt://[user:pass@]host[:port]
    String temp = url;
    
    // 移除协议前缀
    if (temp.startsWith("mqtt://")) {
        temp = temp.substring(7);
    } else if (temp.startsWith("mqtts://")) {
        temp = temp.substring(8);
    }
    
    // 检查是否有认证信息
    int atIndex = temp.indexOf('@');
    if (atIndex > 0) {
        String auth = temp.substring(0, atIndex);
        temp = temp.substring(atIndex + 1);
        
        int colonIndex = auth.indexOf(':');
        if (colonIndex > 0) {
            mqtt_user_ = auth.substring(0, colonIndex);
            mqtt_password_ = auth.substring(colonIndex + 1);
        } else {
            mqtt_user_ = auth;
        }
    }
    
    // 解析主机和端口
    int colonIndex = temp.indexOf(':');
    if (colonIndex > 0) {
        mqtt_server_ = temp.substring(0, colonIndex);
        mqtt_port_ = temp.substring(colonIndex + 1).toInt();
    } else {
        mqtt_server_ = temp;
        mqtt_port_ = 1883;  // 默认端口
    }
    
    Serial.printf("MQTT parsed - Server: %s, Port: %d, User: %s\n", 
                  mqtt_server_.c_str(), mqtt_port_, mqtt_user_.c_str());
    
    return mqtt_server_.length() > 0;
}

bool MqttManager::configure(const String& server, const String& topic)
{
    if (!parseMqttUrl(server)) {
        Serial.println("Failed to parse MQTT server URL");
        return false;
    }
    
    mqtt_topic_ = topic;
    mqtt_client_->setServer(mqtt_server_.c_str(), mqtt_port_);
    
    Serial.printf("MQTT configured - Server: %s:%d, Topic: %s\n", 
                  mqtt_server_.c_str(), mqtt_port_, mqtt_topic_.c_str());
    
    return true;
}

bool MqttManager::reconnect()
{
    if (!mqtt_client_) return false;
    
    Serial.print("Attempting MQTT connection...");
    
    // 创建客户端ID
    String clientId = "ESP32_VFD_" + String(ESP.getEfuseMac(), HEX);
    
    bool connected = false;
    if (mqtt_user_.length() > 0) {
        // 使用用户名密码连接
        connected = mqtt_client_->connect(clientId.c_str(), 
                                         mqtt_user_.c_str(), 
                                         mqtt_password_.c_str());
    } else {
        // 匿名连接
        connected = mqtt_client_->connect(clientId.c_str());
    }
    
    if (connected) {
        Serial.println("connected");
        connected_ = true;
        
        // 订阅主题
        if (mqtt_topic_.length() > 0) {
            subscribe(mqtt_topic_);
        }
        
        return true;
    } else {
        Serial.print("failed, rc=");
        Serial.println(mqtt_client_->state());
        connected_ = false;
        return false;
    }
}

bool MqttManager::connect()
{
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi not connected, cannot connect to MQTT");
        return false;
    }
    
    if (mqtt_server_.length() == 0) {
        Serial.println("MQTT server not configured");
        return false;
    }
    
    return reconnect();
}

void MqttManager::disconnect()
{
    if (mqtt_client_ && mqtt_client_->connected()) {
        mqtt_client_->disconnect();
    }
    connected_ = false;
}

void MqttManager::loop()
{
    if (!mqtt_client_) return;
    
    if (mqtt_client_->connected()) {
        mqtt_client_->loop();
    } else {
        // 每5秒尝试重连一次
        unsigned long now = millis();
        if (now - last_reconnect_attempt_ > 5000) {
            last_reconnect_attempt_ = now;
            
            if (WiFi.status() == WL_CONNECTED) {
                Serial.println("MQTT disconnected, attempting to reconnect...");
                reconnect();
            }
        }
    }
}

bool MqttManager::publish(const String& topic, const String& message)
{
    if (!mqtt_client_ || !mqtt_client_->connected()) {
        return false;
    }
    
    return mqtt_client_->publish(topic.c_str(), message.c_str());
}

bool MqttManager::subscribe(const String& topic)
{
    if (!mqtt_client_ || !mqtt_client_->connected()) {
        return false;
    }
    
    bool success = mqtt_client_->subscribe(topic.c_str());
    if (success) {
        Serial.printf("Subscribed to topic: %s\n", topic.c_str());
    } else {
        Serial.printf("Failed to subscribe to topic: %s\n", topic.c_str());
    }
    
    return success;
}

void MqttManager::messageCallback(char* topic, byte* payload, unsigned int length)
{
    if (instance_) {
        // 转换payload为String
        String message;
        for (unsigned int i = 0; i < length; i++) {
            message += (char)payload[i];
        }
        
        instance_->handleMessage(String(topic), message);
    }
}

void MqttManager::handleMessage(const String& topic, const String& message)
{
    Serial.printf("MQTT Message received - Topic: %s, Message: %s (len=%d)\n", 
                  topic.c_str(), message.c_str(), message.length());
    
    // 调试：显示消息的十六进制表示
    Serial.print("Message hex: ");
    for (int i = 0; i < message.length() && i < 50; i++) {
        Serial.printf("%02X ", (unsigned char)message[i]);
    }
    Serial.println();
    
    // 在VFD上显示消息
    if (vfd_ && vfd_->isInitialized()) {
        vfd_->clear();
        delay(50);
        
        // 处理消息中的换行符
        // 支持的换行符格式：
        // 1. 字符 '\n' (0x0A) - 真实的换行符
        // 2. 字符 '|' (0x7C) - 竖线作为换行分隔符
        
        String displayMessage = message;
        
        // 替换竖线为换行符
        for (int i = 0; i < displayMessage.length(); i++) {
            if (displayMessage[i] == '|') {
                displayMessage.setCharAt(i, '\n');
            }
        }
        
        Serial.printf("After processing: %d chars\n", displayMessage.length());
        
        // 按换行符分割并显示
        int startPos = 0;
        int lineCount = 0;
        const int maxLines = 2;  // VFD最多显示行数
        const int maxLineLength = 20;  // 每行最大字符数
        
        while (startPos < displayMessage.length() && lineCount < maxLines) {
            // 查找下一个换行符
            int newlinePos = displayMessage.indexOf('\n', startPos);
            
            Serial.printf("Line %d: startPos=%d, newlinePos=%d\n", lineCount, startPos, newlinePos);
            
            // 提取当前行
            String line;
            if (newlinePos == -1) {
                // 没有找到换行符，取剩余所有内容
                line = displayMessage.substring(startPos);
            } else {
                // 取到换行符之前的内容
                line = displayMessage.substring(startPos, newlinePos);
            }
            
            Serial.printf("  Line content: '%s' (len=%d)\n", line.c_str(), line.length());
            
            // 限制行长度
            if (line.length() > maxLineLength) {
                line = line.substring(0, maxLineLength);
            }
            
            // 显示当前行
            if (lineCount == 0) {
                // 第一行：直接打印
                vfd_->print(line);
                Serial.printf("  Line 0: printed '%s'\n", line.c_str());
            } else {
                // 第二行：发送回车+换行，让光标到下一行行首
                Serial.println("  Sending CR+LF (0x0D 0x0A)");
                vfd_->print("\r\n");  // 回车+换行
                delay(20);
                vfd_->print(line);
                Serial.printf("  Line %d: CR+LF + printed '%s'\n", lineCount, line.c_str());
            }
            
            lineCount++;
            
            // 如果没有找到换行符，说明已经是最后的内容了
            if (newlinePos == -1) {
                break;
            }
            
            // 移动到下一行的起始位置
            startPos = newlinePos + 1;
        }
        
        Serial.printf("Message displayed on VFD (%d lines)\n", lineCount);
    }
}
