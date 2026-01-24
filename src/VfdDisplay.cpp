#include "VfdDisplay.h"

VfdDisplay::VfdDisplay(HardwareSerial *serial, int rx_pin, int tx_pin, int baud_rate)
    : serial_(serial), rx_pin_(rx_pin), tx_pin_(tx_pin), baud_rate_(baud_rate), initialized_(false)
{
}

VfdDisplay::~VfdDisplay()
{
    if (initialized_ && serial_) {
        serial_->end();
    }
}

bool VfdDisplay::begin()
{
    if (!serial_) {
        Serial.println("VFD: Serial pointer is null");
        return false;
    }

    Serial.printf("VFD: Initializing on RX=%d, TX=%d, Baud=%d\n", rx_pin_, tx_pin_, baud_rate_);
    
    // 初始化串口
    serial_->begin(baud_rate_, SERIAL_8N1, rx_pin_, tx_pin_);
    delay(100);  // 等待串口稳定
    
    // 初始化VFD显示屏
    clear();
    delay(50);
    
    displayOn();
    delay(50);
    
    cursorOff();
    delay(50);
    
    setBrightness(100);  // 设置为最大亮度
    delay(50);
    
    initialized_ = true;
    Serial.println("VFD: Initialization complete");
    
    return true;
}

void VfdDisplay::sendEscCommand(uint8_t cmd)
{
    if (!serial_) return;
    serial_->write(VFD_ESC);
    serial_->write(cmd);
}

void VfdDisplay::sendEscCommand(uint8_t cmd, uint8_t param)
{
    if (!serial_) return;
    serial_->write(VFD_ESC);
    serial_->write(cmd);
    serial_->write(param);
}

void VfdDisplay::sendEscCommand(uint8_t cmd, const uint8_t* params, size_t len)
{
    if (!serial_) return;
    serial_->write(VFD_ESC);
    serial_->write(cmd);
    for (size_t i = 0; i < len; i++) {
        serial_->write(params[i]);
    }
}

void VfdDisplay::clear()
{
    if (!serial_) return;
    serial_->write(VFD_CLEAR_DISPLAY);
    delay(10);  // 清屏需要一些时间
}

void VfdDisplay::home()
{
    if (!serial_) return;
    serial_->write(VFD_CURSOR_HOME);
}

void VfdDisplay::print(const char* text)
{
    if (!serial_ || !text) return;
    serial_->print(text);
}

void VfdDisplay::print(String text)
{
    if (!serial_) return;
    serial_->print(text);
}

void VfdDisplay::println(const char* text)
{
    print(text);
    serial_->write(VFD_LINE_FEED);
}

void VfdDisplay::println(String text)
{
    print(text);
    serial_->write(VFD_LINE_FEED);
}

void VfdDisplay::setCursor(uint8_t x, uint8_t y)
{
    if (!serial_) return;
    // US $ x y - 设置光标位置 (US = 0x1F)
    // x: 列号 (0-19), y: 行号
    serial_->write(VFD_US);
    serial_->write('$');
    serial_->write(x);
    serial_->write(y);
}

void VfdDisplay::cursorOn()
{
    if (!serial_) return;
    // ESC C 1 - 显示光标
    sendEscCommand('C', 1);
}

void VfdDisplay::cursorOff()
{
    if (!serial_) return;
    // ESC C 0 - 隐藏光标
    sendEscCommand('C', 0);
}

void VfdDisplay::setBrightness(uint8_t level)
{
    if (!serial_) return;
    
    // EPSON VFD通常支持4级亮度 (25%, 50%, 75%, 100%)
    // ESC L n - 设置亮度等级
    uint8_t brightness_level;
    
    if (level <= 25) {
        brightness_level = 1;
    } else if (level <= 50) {
        brightness_level = 2;
    } else if (level <= 75) {
        brightness_level = 3;
    } else {
        brightness_level = 4;
    }
    
    sendEscCommand('L', brightness_level);
}

void VfdDisplay::displayOn()
{
    if (!serial_) return;
    // ESC D 1 - 打开显示
    sendEscCommand('D', 1);
}

void VfdDisplay::displayOff()
{
    if (!serial_) return;
    // ESC D 0 - 关闭显示
    sendEscCommand('D', 0);
}

void VfdDisplay::test()
{
    if (!initialized_) {
        Serial.println("VFD: Not initialized");
        return;
    }
    
    Serial.println("VFD: Running test sequence");
    
    // 清屏
    clear();
    delay(500);
    
    // 测试基本文本显示
    print("VFD Test OK");
    delay(2000);
    
    // 测试换行
    clear();
    println("Line 1");
    println("Line 2");
    delay(2000);
    
    // 测试亮度
    for (int i = 100; i >= 25; i -= 25) {
        setBrightness(i);
        delay(500);
    }
    setBrightness(100);
    
    clear();
    print("Test Complete!");
    
    Serial.println("VFD: Test complete");
}
