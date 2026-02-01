#include "MqttManager.h"

// 静态成员初始化
MqttManager* MqttManager::instance_ = nullptr;

MqttManager::MqttManager()
    : connected_(false), last_reconnect_attempt_(0),
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
        int port = temp.substring(colonIndex + 1).toInt();
        mqtt_port_ = (port > 0 && port <= 65535) ? port : 1883;
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
    Serial.printf("MQTT Message: [%s] %s\n", topic.c_str(), message.c_str());
    
    if (onMessage_) {
        onMessage_(topic, message);
    }
}
