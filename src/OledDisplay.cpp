#include "OledDisplay.h"

OledDisplay::OledDisplay(int width, int height, int sda_pin, int scl_pin, uint8_t i2c_addr)
    : width_(width), height_(height), sda_pin_(sda_pin), scl_pin_(scl_pin), 
      i2c_addr_(i2c_addr), initialized_(false), display_(nullptr),
      cursor_x_(0), cursor_y_(0), line_height_(10)
{
}

OledDisplay::~OledDisplay()
{
    if (display_) {
        delete display_;
        display_ = nullptr;
    }
}

bool OledDisplay::begin()
{
    Serial.println("=== OLED Display Initialization (U8g2) ===");
    Serial.printf("Configuration: %dx%d, SDA=%d, SCL=%d, I2C=0x%02X\n", 
                  width_, height_, sda_pin_, scl_pin_, i2c_addr_);
    
    // 初始化 I2C
    Wire.begin(sda_pin_, scl_pin_);
    
    // 创建 U8g2 显示对象 (72x40 OLED)
    display_ = new U8G2_SSD1306_72X40_ER_F_HW_I2C(U8G2_R0, U8X8_PIN_NONE);
    
    if (!display_) {
        Serial.println("ERROR: Failed to allocate U8g2 display");
        return false;
    }
    
    // 初始化显示屏
    display_->begin();
    display_->setI2CAddress(i2c_addr_ << 1); // U8g2 uses 8-bit address
    
    initialized_ = true;
    
    // 设置字体
    display_->setFont(u8g2_font_6x10_tf); // 6x10 字体
    line_height_ = 10;
    
    // 清屏
    display_->clearBuffer();
    display_->sendBuffer();
    
    Serial.printf("OLED Display initialized: %dx%d @ 0x%02X\n", width_, height_, i2c_addr_);
    Serial.println("==========================================");
    
    return true;
}

void OledDisplay::clear()
{
    if (!display_) return;
    
    display_->clearBuffer();
    display_->sendBuffer();
    cursor_x_ = 0;
    cursor_y_ = 0;
}

void OledDisplay::home()
{
    cursor_x_ = 0;
    cursor_y_ = 0;
}

void OledDisplay::print(const char* text)
{
    if (!display_ || !text) return;
    
    display_->drawStr(cursor_x_, cursor_y_ + line_height_, text);
    display_->sendBuffer();
    
    // 更新光标位置
    cursor_x_ += display_->getStrWidth(text);
}

void OledDisplay::print(String text)
{
    print(text.c_str());
}

void OledDisplay::println(const char* text)
{
    if (!display_ || !text) return;
    
    display_->drawStr(cursor_x_, cursor_y_ + line_height_, text);
    display_->sendBuffer();
    
    // 换行
    cursor_x_ = 0;
    cursor_y_ += line_height_;
}

void OledDisplay::println(String text)
{
    println(text.c_str());
}

void OledDisplay::setCursor(uint8_t x, uint8_t y)
{
    if (!display_) return;
    
    // 将字符坐标转换为像素坐标
    cursor_x_ = x * 6;  // 假设字符宽度 6 像素
    cursor_y_ = y * line_height_;
}

void OledDisplay::setBrightness(uint8_t level)
{
    if (!display_) return;
    
    // U8g2 的对比度控制
    display_->setContrast(level);
}

void OledDisplay::displayOn()
{
    if (!display_) return;
    display_->setPowerSave(0);
}

void OledDisplay::displayOff()
{
    if (!display_) return;
    display_->setPowerSave(1);
}

void OledDisplay::test()
{
    if (!display_) return;
    
    Serial.println("=== OLED Display Test (U8g2) ===");
    
    // 测试 1: 绘制边框
    Serial.println("  - Drawing border");
    display_->clearBuffer();
    display_->drawFrame(0, 0, width_, height_);
    display_->sendBuffer();
    delay(1500);
    
    // 测试 2: 显示文本
    Serial.println("  - Displaying text");
    display_->clearBuffer();
    display_->setFont(u8g2_font_6x10_tf);
    display_->drawStr(0, 10, "72x40");
    display_->drawStr(0, 20, "U8g2 OK");
    display_->sendBuffer();
    delay(2000);
    
    // 测试 3: 全屏填充
    Serial.println("  - Fill screen");
    display_->clearBuffer();
    display_->drawBox(0, 0, width_, height_);
    display_->sendBuffer();
    delay(500);
    
    // 测试 4: 清屏
    Serial.println("  - Clear screen");
    display_->clearBuffer();
    display_->sendBuffer();
    
    Serial.println("=== Test Complete ===");
}
