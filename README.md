# ESP32-C3 VFD Display 项目

ESP32-C3控制EPSON VFD显示屏的完整IoT项目，集成WebServer配置界面和MQTT实时消息显示。

## 核心功能

✅ **VFD显示控制** - 串口控制EPSON VFD显示屏  
✅ **WiFi管理** - 自动连接WiFi，支持AP配置模式  
✅ **Web配置界面** - 通过浏览器配置WiFi和MQTT  
✅ **MQTT订阅** - 实时接收MQTT消息并显示在VFD上  
✅ **持久化配置** - 配置保存在SPIFFS中  

## 硬件连接

### ESP32-C3 引脚定义

- **VFD显示屏**: GPIO 10 (TX) → VFD RX，波特率 9600
- **LED指示灯**: GPIO 8 (可选)
- **USB串口**: 用于调试输出 (115200)

### VFD显示屏连接

```
ESP32-C3 GPIO 10 (TX) ──→ VFD RX
ESP32-C3 GND          ──→ VFD GND
ESP32-C3 5V/3.3V      ──→ VFD VCC (根据VFD规格)
```

## 功能特性

### 1. VFD显示控制
- ✅ 串口通信 (9600波特率)
- ✅ 显示文本消息
- ✅ 清屏、光标控制
- ✅ 亮度调节 (4级)
- ✅ 显示开关控制
- ✅ MQTT消息实时显示

### 2. WiFi管理
- ✅ 自动连接WiFi
- ✅ AP配置模式（首次启动或连接失败）
- ✅ 看门狗自动重连
- ✅ 连接成功后显示IP地址

### 3. Web配置界面
- ✅ 响应式Web界面
- ✅ WiFi SSID/密码配置
- ✅ MQTT服务器配置
- ✅ MQTT主题订阅配置
- ✅ 实时状态显示
- ✅ 配置持久化

### 4. MQTT功能
- ✅ 自动连接MQTT服务器
- ✅ 订阅指定主题
- ✅ 实时接收消息
- ✅ 消息自动显示在VFD上
- ✅ 支持用户名密码认证
- ✅ 自动重连机制

### 5. 文件系统
- ✅ SPIFFS配置存储
- ✅ WiFi配置持久化
- ✅ MQTT配置持久化

## 编译和上传

### 编译项目
```bash
cd /Users/nate/Documents/GitHub/EspDotDisplay
pio run
```

### 上传固件
```bash
pio run -t upload
```

### 监控串口输出
```bash
pio device monitor
```

## 使用说明

### 1. 首次启动 - AP配置模式

程序首次启动时会进入AP配置模式：

1. **VFD显示**: "AP Mode" + IP地址
2. **连接WiFi热点**:
   - SSID: `ESP_Config_xxxxxx` (xxxxxx是芯片ID)
   - 密码: `12345678`
3. **打开浏览器访问**: `http://192.168.4.1`
4. **配置WiFi和MQTT**:
   - WiFi SSID: 你的WiFi名称
   - WiFi 密码: 你的WiFi密码
   - MQTT服务器: `mqtt://user:pass@host:port` 或 `mqtt://host:port`
   - MQTT主题: `/your/topic/path`
5. **保存配置**: 设备会自动重启并连接WiFi

### 2. WiFi连接成功

连接成功后：
1. **VFD显示**: "WiFi OK" + IP地址（2秒）
2. **浏览器访问**: `http://设备IP地址`
3. **可以重新配置**: 点击"WiFi & MQTT 配置"按钮

### 3. MQTT连接和消息显示

WiFi连接后自动：
1. **连接MQTT服务器**
2. **订阅配置的主题**
3. **VFD显示**: "MQTT OK" + "Listening..."
4. **接收消息**: 实时显示在VFD上

### 4. MQTT消息示例

使用mosquitto_pub测试：
```bash
# 发送简单消息
mosquitto_pub -h mqtt.server.com -t /your/topic/path -m "Hello VFD"

# 使用认证
mosquitto_pub -h mqtt.server.com -u username -P password -t /your/topic/path -m "Test Message"
```

消息会立即显示在VFD屏幕上！

## VFD控制API

### 基本显示
```cpp
vfd.clear();              // 清屏
vfd.print("Hello");       // 显示文本
vfd.println("Line 1");    // 显示文本并换行
```

### 光标控制
```cpp
vfd.setCursor(0, 0);      // 设置光标位置
vfd.cursorOn();           // 显示光标
vfd.cursorOff();          // 隐藏光标
```

### 显示控制
```cpp
vfd.setBrightness(100);   // 设置亮度 (25, 50, 75, 100)
vfd.displayOn();          // 打开显示
vfd.displayOff();         // 关闭显示
```

## 项目结构

```
EspDotDisplay/
├── platformio.ini              # PlatformIO配置
├── src/
│   ├── main.cpp               # 主程序（集成所有功能）
│   ├── VfdDisplay.h           # VFD显示屏头文件
│   ├── VfdDisplay.cpp         # VFD显示屏实现
│   ├── EspSmartWifi.h         # WiFi管理头文件
│   ├── EspSmartWifi.cpp       # WiFi管理实现
│   ├── ConfigWebServer.h      # Web配置服务器头文件
│   ├── ConfigWebServer.cpp    # Web配置服务器实现
│   ├── MqttManager.h          # MQTT管理头文件
│   └── MqttManager.cpp        # MQTT管理实现
└── README.md                  # 本文档
```

## 系统架构

```
┌─────────────────────────────────────────────────────┐
│                   ESP32-C3 主控                      │
│                                                     │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐         │
│  │   WiFi   │  │WebServer │  │   MQTT   │         │
│  │  Manager │  │ (配置页) │  │ Manager  │         │
│  └─────┬────┘  └────┬─────┘  └────┬─────┘         │
│        │            │             │                │
│        └────────────┴─────────────┘                │
│                     │                              │
│              ┌──────┴───────┐                      │
│              │ VFD Display  │                      │
│              │   Manager    │                      │
│              └──────┬───────┘                      │
└─────────────────────┼──────────────────────────────┘
                      │
                      ▼
              ┌───────────────┐
              │  EPSON VFD    │
              │   Display     │
              └───────────────┘
```

## 编译信息

- **平台**: ESP32-C3
- **框架**: Arduino
- **RAM使用**: 12.0% (39,428 / 327,680 字节)
- **Flash使用**: 66.7% (873,940 / 1,310,720 字节)

## 依赖库

- **ArduinoJson 6.21.5** - JSON配置解析
- **PubSubClient 2.8.0** - MQTT客户端库

## 调试输出

通过USB串口可以看到详细的调试信息：
- VFD初始化状态
- WiFi连接过程
- IP地址信息
- Web服务器状态
- MQTT连接状态
- 接收到的MQTT消息
- 定期状态更新（每30秒）

串口输出示例：
```
=================================
ESP32-C3 VFD Display System
with WebServer & MQTT
=================================

Initializing VFD Display...
VFD: Initializing on RX=-1, TX=10, Baud=9600
VFD: Initialization complete
VFD Display initialized successfully

Web server started on port 80
WiFi Status: Connected
IP Address: 192.168.1.100
Web interface: http://192.168.1.100

Configuring MQTT...
MQTT configured successfully
Attempting MQTT connection...connected
Subscribed to topic: /test/vfd
MQTT connected and subscribed
```

## Web配置界面

### 主页面
- 显示设备在线状态
- 显示WiFi连接状态
- 显示IP地址
- 进入配置页面的按钮

### 配置页面
表单字段：
- **WiFi SSID**: 要连接的WiFi名称
- **WiFi 密码**: WiFi密码
- **MQTT 服务器**: 格式支持：
  - `mqtt://host` (无认证，默认端口1883)
  - `mqtt://host:port` (无认证，指定端口)
  - `mqtt://user:pass@host` (有认证，默认端口)
  - `mqtt://user:pass@host:port` (有认证，指定端口)
- **MQTT 主题**: 要订阅的主题路径

### 状态API
GET `/status` 返回JSON：
```json
{
  "connected": true,
  "ip": "192.168.1.100",
  "rssi": -45,
  "ap_mode": false
}
```

支持的EPSON VFD标准命令：
- `0x0C` - 清屏
- `0x0B` - 光标回到起始位置
- `0x08` - 退格
- `0x09` - 水平制表符
- `0x0A` - 换行
- `0x0D` - 回车
- `ESC C n` - 光标控制 (n=0:隐藏, n=1:显示)
- `ESC L n` - 亮度控制 (n=1-4)
- `ESC D n` - 显示开关 (n=0:关, n=1:开)
- `US $ x y` - 设置光标位置

## 注意事项

1. **引脚配置**: 确保GPIO 10未被其他功能占用
2. **波特率**: VFD默认9600，如需修改请在main.cpp中调整`VFD_BAUD_RATE`
3. **电源**: 确认VFD的工作电压(3.3V或5V)与ESP32-C3输出匹配
4. **调试**: USB串口(Serial)用于调试，VFD串口(Serial1)用于显示控制
5. **MQTT**: 确保MQTT服务器可访问，检查防火墙设置
6. **网络**: AP模式下Web界面地址为 `192.168.4.1`，STA模式下使用实际IP
7. **消息长度**: VFD显示限制，过长消息会被截断（目前最多40字符）

## 完整使用流程

1. **上传固件** → 2. **连接AP** → 3. **配置WiFi/MQTT** → 4. **自动连接** → 5. **发送MQTT消息** → 6. **VFD实时显示**

```mermaid
graph LR
A[上传固件] --> B[设备进入AP模式]
B --> C[连接ESP_Config_xxx]
C --> D[访问192.168.4.1]
D --> E[填写WiFi和MQTT配置]
E --> F[保存并重启]
F --> G[连接WiFi]
G --> H[连接MQTT]
H --> I[订阅主题]
I --> J[接收消息]
J --> K[VFD显示]
```

## 下一步开发

可以添加的功能：
- [ ] MQTT消息滚动显示（支持长消息）
- [ ] 多主题订阅
- [ ] 消息过滤规则
- [ ] 定时任务和提醒
- [ ] OTA固件升级
- [ ] 自定义字符和图标
- [ ] REST API控制
- [ ] 消息历史记录
- [ ] VFD亮度自动调节
- [ ] 断线提醒

## API参考

### VfdDisplay类
```cpp
vfd.begin();                    // 初始化
vfd.clear();                    // 清屏
vfd.print("text");             // 显示文本
vfd.println("text");           // 显示文本并换行
vfd.setCursor(x, y);           // 设置光标
vfd.setBrightness(level);      // 设置亮度(1-4或25-100)
vfd.displayOn/Off();           // 显示开关
```

### MqttManager类
```cpp
mqtt.configure(server, topic);  // 配置MQTT
mqtt.connect();                 // 连接
mqtt.subscribe(topic);          // 订阅主题
mqtt.publish(topic, msg);       // 发布消息
mqtt.loop();                    // 消息循环（在loop中调用）
mqtt.isConnected();             // 检查连接状态
```

### ConfigWebServer类
```cpp
webServer.begin();              // 启动服务器
webServer.handleClient();       // 处理请求（在loop中调用）
webServer.stop();               // 停止服务器
```

## 故障排除

### VFD无显示
- 检查GPIO 10连接
- 确认波特率9600
- 检查VFD电源
- 查看串口调试信息

### WiFi连接失败
- 检查SSID和密码
- 确认WiFi信号强度
- 尝试重新配置

### MQTT连接失败
- 检查服务器地址和端口
- 确认用户名密码
- 检查网络连通性
- 查看MQTT服务器日志

### Web界面无法访问
- 确认设备IP地址
- 检查是否在同一网络
- 尝试重启设备

## 许可证

MIT License
