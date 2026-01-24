#pragma once
#include <Arduino.h>
#include <FS.h>
#include <SPIFFS.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WebServer.h>
#include <ArduinoJson.h>

struct Config {
  String SSID = "1";
  String Passwd = "1";
  String Server = "mqtt://username:passwd@mqtt.server";  // MQTT服务器地址，支持认证
  String Topic = "/espVfd/message";  // MQTT主题
  bool bConfigValid = false;
};

struct EMPTY_SERIAL
{
  void println(const char *){}
  void println(String){}
  void printf(const char *, ...){}
  void print(const char *){}
  //void print(Printable) {}
  void begin(int){}
  void end(){}
};
//_EMPTY_SERIAL _EMPTY_SERIAL;
//#define Serial_debug  _EMPTY_SERIAL
#define Serial_debug  Serial

class EspSmartWifi
{
private:
    int led_pin_;
    fs::File root;
    Config _config;
    bool _isAPMode;
    bool cachedRelayStates[6] = {false};  // 缓存继电器状态
    bool statesLoaded = false;  // 标记是否已加载状态

    void BaseConfig();

    bool LoadConfig();
    bool SaveConfig();
    
    // 继电器状态相关方法
    bool loadRelayStatesFromFlash();  // 从flash加载状态到缓存
    bool saveRelayStatesToFlash();    // 将缓存保存到flash
    
    // LED控制方法
    void ledBlink(int times, int onMs = 100, int offMs = 100);

public:
    EspSmartWifi(int led_pin = -1):
    led_pin_(led_pin), _isAPMode(false)
    {
        if (led_pin_ >= 0) {
            pinMode(led_pin_, OUTPUT);
            digitalWrite(led_pin_, LOW);
        }
    }
    ~EspSmartWifi(){

    }

    void initFS();
    bool WiFiWatchDog();
    void ConnectWifi();
    void DisplayIP();
    
    // HTTP client methods
    String httpGet(const String& path);
    const Config& getConfig() const { return _config; }
    WiFiClient client;

    // 继电器状态相关方法
    bool updateRelayState(int index, bool state);
    bool getRelayStates(bool states[6]);
    void syncRelayStates();  // 同步继电器状态到硬件
    bool SaveConfig(Config config);
    // 获取AP模式状态
    bool isAPMode() const { return _isAPMode; }

    void StartAPMode();
    void StopAPMode();
    void TryConnectWifi();
};




