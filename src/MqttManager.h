#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include "VfdDisplay.h"

class MqttManager
{
private:
    WiFiClient *wifi_client_;
    PubSubClient *mqtt_client_;
    VfdDisplay *vfd_;
    
    String mqtt_server_;
    int mqtt_port_;
    String mqtt_user_;
    String mqtt_password_;
    String mqtt_topic_;
    
    bool connected_;
    unsigned long last_reconnect_attempt_;
    
    // 解析MQTT服务器URL (mqtt://user:pass@host:port)
    bool parseMqttUrl(const String& url);
    
    // 重连尝试
    bool reconnect();
    
    // MQTT消息回调（静态）
    static void messageCallback(char* topic, byte* payload, unsigned int length);
    
    // 实例指针（用于回调）
    static MqttManager* instance_;

public:
    MqttManager(VfdDisplay *vfd);
    ~MqttManager();
    
    // 配置MQTT
    bool configure(const String& server, const String& topic);
    
    // 连接MQTT
    bool connect();
    
    // 断开连接
    void disconnect();
    
    // 循环处理（在loop中调用）
    void loop();
    
    // 发布消息
    bool publish(const String& topic, const String& message);
    
    // 订阅主题
    bool subscribe(const String& topic);
    
    // 检查是否已连接
    bool isConnected() const { return connected_ && mqtt_client_ && mqtt_client_->connected(); }
    
    // 处理接收到的消息
    void handleMessage(const String& topic, const String& message);
};
