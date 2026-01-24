# MQTT消息换行使用指南

## 推荐使用方式

**强烈推荐使用竖线 `|` 作为换行符**，这是最简单可靠的方式！

## 支持的换行方式

ESP32-C3 VFD Display系统支持两种方式发送带换行的MQTT消息：

### 1. 使用竖线 `|` 分隔（强烈推荐✅）

```bash
# 发送两行消息
mosquitto_pub -t /espVfd/message -m "Line 1|Line 2"

# 示例
mosquitto_pub -t /espVfd/message -m "Hello|World"
```

**VFD显示效果：**
```
Hello
World
```

### 2. 真实的换行符（需要特殊处理）

如果你的MQTT客户端发送的消息中包含真实的换行符（ASCII 0x0A），也会被识别。

```bash
# 使用echo和管道
echo -e "Line 1\nLine 2" | mosquitto_pub -t /espVfd/message -s
```

## MQTT客户端示例

### 使用mosquitto_pub（推荐竖线方式）

```bash
# 基本用法（无认证）- 使用竖线分隔
mosquitto_pub -h mqtt.server.com -t /espVfd/message -m "Hello|World"

# 带用户名密码
mosquitto_pub -h mqtt.server.com -u username -P password -t /espVfd/message -m "Line1|Line2"

# 指定端口
mosquitto_pub -h mqtt.server.com -p 1883 -t /espVfd/message -m "Text|More"

# 使用echo发送真实换行符
echo -e "Line1\nLine2" | mosquitto_pub -h mqtt.server.com -t /espVfd/message -s
```

### 使用Python (paho-mqtt)

```python
import paho.mqtt.client as mqtt

client = mqtt.Client()
client.connect("mqtt.server.com", 1883, 60)

# 推荐：使用竖线
client.publish("/espVfd/message", "Line 1|Line 2")

# 或使用真实换行符
client.publish("/espVfd/message", "Line 1\nLine 2")

client.disconnect()
```

### 使用Node.js (mqtt)

```javascript
const mqtt = require('mqtt');
const client = mqtt.connect('mqtt://mqtt.server.com');

client.on('connect', () => {
  // 推荐：使用竖线
  client.publish('/espVfd/message', 'Line 1|Line 2');
  
  // 或使用真实换行符
  client.publish('/espVfd/message', 'Line 1\nLine 2');
  
  client.end();
});
```

## 显示规则

1. **最大行数**: 2行（根据VFD屏幕规格）
2. **每行最大字符数**: 20字符
3. **超长处理**: 
   - 超过20字符的行会被截断
   - 超过2行的内容会被忽略

## 实际示例

### 示例1: 显示传感器数据

```bash
mosquitto_pub -t /espVfd/message -m "Temp: 25.5°C|Humi: 65%"
```

**显示：**
```
Temp: 25.5°C
Humi: 65%
```

### 示例2: 显示时间和日期

```bash
mosquitto_pub -t /espVfd/message -m "2024-01-24|15:30:45"
```

**显示：**
```
2024-01-24
15:30:45
```

### 示例3: 显示状态信息

```bash
mosquitto_pub -t /espVfd/message -m "System Status|All OK"
```

**显示：**
```
System Status
All OK
```

### 示例4: 中文显示（如果VFD支持）

```bash
mosquitto_pub -t /espVfd/message -m "欢迎使用|VFD显示屏"
```

**显示：**
```
欢迎使用
VFD显示屏
```

## 调试技巧

### 查看收到的消息

通过USB串口监视器（115200波特率）可以看到：

```
MQTT Message received - Topic: /espVfd/message, Message: Line1|Line2
Message displayed on VFD (2 lines)
```

### 测试不同格式

```bash
# 测试单行
mosquitto_pub -t /espVfd/message -m "Single Line"

# 测试两行
mosquitto_pub -t /espVfd/message -m "Line 1|Line 2"

# 测试长文本
mosquitto_pub -t /espVfd/message -m "Very Long Text That Will Be Truncated|Second Line"
```

## 常见问题

### Q: 为什么我的消息没有换行？

A: 确保使用了正确的分隔符：
- 使用 `|` （竖线）最简单
- 或使用 `\n` 但要确保shell正确解析

### Q: 可以显示超过2行吗？

A: 不可以，VFD屏幕限制为2行，多余的行会被忽略。

### Q: 每行可以显示多少字符？

A: 默认每行最多20个字符，超过部分会被截断。实际字符数取决于VFD屏幕规格。

### Q: 支持哪些字符？

A: 取决于VFD显示屏支持的字符集，通常支持ASCII字符，部分VFD支持中文。

## 高级用法

### 定时更新显示

使用cron定时发送消息：

```bash
# 每分钟更新时间
*/1 * * * * mosquitto_pub -t /espVfd/message -m "$(date +%H:%M:%S)|$(date +%Y-%m-%d)"
```

### Shell脚本示例

```bash
#!/bin/bash
TOPIC="/espVfd/message"
BROKER="mqtt.server.com"

while true; do
    TEMP=$(sensors | grep 'temp1' | awk '{print $2}')
    TIME=$(date +%H:%M)
    
    mosquitto_pub -h $BROKER -t $TOPIC -m "Temp: $TEMP|Time: $TIME"
    
    sleep 60
done
```

## 配置说明

在Web配置界面中设置：

1. **MQTT服务器**: `mqtt://user:pass@host:port`
2. **MQTT主题**: `/espVfd/message` （或自定义）

消息会自动显示在VFD屏幕上！
