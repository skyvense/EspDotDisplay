#pragma once
#include <Arduino.h>

/**
 * @brief 显示接口基类
 * 
 * 定义通用的显示接口，VFD 和 OLED 都实现此接口
 */
class DisplayInterface
{
public:
    virtual ~DisplayInterface() {}
    
    // 初始化
    virtual bool begin() = 0;
    
    // 基本显示控制
    virtual void clear() = 0;
    virtual void home() = 0;
    virtual void print(const char* text) = 0;
    virtual void print(String text) = 0;
    virtual void println(const char* text) = 0;
    virtual void println(String text) = 0;
    
    // 光标控制
    virtual void setCursor(uint8_t x, uint8_t y) = 0;
    
    // 显示控制
    virtual void setBrightness(uint8_t level) = 0;
    virtual void displayOn() = 0;
    virtual void displayOff() = 0;
    
    // 测试
    virtual void test() = 0;
    
    // 状态查询
    virtual bool isInitialized() const = 0;
};
