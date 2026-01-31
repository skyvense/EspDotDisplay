#pragma once
#include <Arduino.h>
#include <HardwareSerial.h>
#include "DisplayInterface.h"

// EPSON VFD 控制命令
#define VFD_CLEAR_DISPLAY       0x0C    // 清屏
#define VFD_CURSOR_HOME         0x0B    // 光标回到起始位置
#define VFD_BACKSPACE           0x08    // 退格
#define VFD_HORIZONTAL_TAB      0x09    // 水平制表符
#define VFD_LINE_FEED           0x0A    // 换行
#define VFD_CARRIAGE_RETURN     0x0D    // 回车
#define VFD_ESC                 0x1B    // ESC控制字符
#define VFD_US                  0x1F    // US控制字符

class VfdDisplay : public DisplayInterface
{
private:
    HardwareSerial *serial_;
    int rx_pin_;
    int tx_pin_;
    int baud_rate_;
    bool initialized_;
    int offset_x_;
    int offset_y_;

    // 发送ESC命令
    void sendEscCommand(uint8_t cmd);
    // 发送ESC命令带参数
    void sendEscCommand(uint8_t cmd, uint8_t param);
    // 发送ESC命令带多个参数
    void sendEscCommand(uint8_t cmd, const uint8_t* params, size_t len);

public:
    VfdDisplay(HardwareSerial *serial = &Serial1,
               int rx_pin = 18,
               int tx_pin = 19,
               int baud_rate = 9600,
               int offset_x = 0,
               int offset_y = 0);
    ~VfdDisplay();

    // 初始化VFD
    bool begin();
    
    // 基本显示控制
    void clear();                           // 清屏
    void home();                            // 光标回到起始位置
    void print(const char* text);           // 打印文本
    void print(String text);                // 打印String
    void println(const char* text);         // 打印文本并换行
    void println(String text);              // 打印String并换行
    
    // 光标控制
    void setCursor(uint8_t x, uint8_t y);   // 设置光标位置（行列均为 1-based，内部转协议 0-based）
    void cursorOn();                        // 显示光标
    void cursorOff();                       // 隐藏光标
    
    // 亮度控制 (0-100%)
    void setBrightness(uint8_t level);      // 设置亮度 (1-4级或0-100%)
    
    // 显示开关
    void displayOn();                       // 打开显示
    void displayOff();                      // 关闭显示
    
    // 测试功能
    void test();                            // 显示测试信息
    
    // 检查是否已初始化
    bool isInitialized() const { return initialized_; }
};
