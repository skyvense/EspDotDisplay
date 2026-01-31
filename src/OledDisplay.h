#pragma once
#include <Arduino.h>
#include <Wire.h>
#include <U8g2lib.h>
#include "DisplayInterface.h"

/**
 * @brief OLED 显示屏控制类 (使用 U8g2 库)
 * 
 * 支持 72x40 OLED 屏幕 (SSD1306)
 * 使用 I2C 接口
 */
class OledDisplay : public DisplayInterface
{
private:
    U8G2 *display_;
    int sda_pin_;
    int scl_pin_;
    int width_;
    int height_;
    uint8_t i2c_addr_;
    bool initialized_;
    int offset_x_;
    int offset_y_;
    
    int cursor_x_;
    int cursor_y_;
    int line_height_;

public:
    /**
     * @brief 构造函数
     * @param width 屏幕宽度（像素）
     * @param height 屏幕高度（像素）
     * @param sda_pin SDA 引脚
     * @param scl_pin SCL 引脚
     * @param i2c_addr I2C 地址（默认 0x3C）
     */
    OledDisplay(int width = 72,
                int height = 40,
                int sda_pin = 5,
                int scl_pin = 6,
                uint8_t i2c_addr = 0x3C,
                int offset_x = 0,
                int offset_y = 0);
    
    ~OledDisplay();
    
    /**
     * @brief 初始化 OLED 显示屏
     * @return true 成功，false 失败
     */
    bool begin();
    
    /**
     * @brief 清屏
     */
    void clear();
    
    /**
     * @brief 回到起始位置
     */
    void home();
    
    /**
     * @brief 打印文本（不换行）
     * @param text 要打印的文本
     */
    void print(const char* text);
    void print(String text);
    
    /**
     * @brief 打印文本并换行
     * @param text 要打印的文本
     */
    void println(const char* text);
    void println(String text);
    
    /**
     * @brief 设置光标位置（字符坐标）
     * @param x 列号（0-based）
     * @param y 行号（0-based）
     */
    void setCursor(uint8_t x, uint8_t y);
    
    /**
     * @brief 设置亮度
     * @param level 亮度等级 (0-255)
     */
    void setBrightness(uint8_t level);
    
    /**
     * @brief 显示开启
     */
    void displayOn();
    
    /**
     * @brief 显示关闭
     */
    void displayOff();
    
    /**
     * @brief 测试显示
     */
    void test();
    
    /**
     * @brief 检查是否已初始化
     */
    bool isInitialized() const { return initialized_; }
    
    /**
     * @brief 获取屏幕宽度
     */
    int getWidth() const { return width_; }
    
    /**
     * @brief 获取屏幕高度
     */
    int getHeight() const { return height_; }
};
