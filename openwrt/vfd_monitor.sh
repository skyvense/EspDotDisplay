#!/bin/sh
#
# VFD Display Monitor Script for OpenWrt
# 功能：每秒推送当前时间和网络延迟到VFD显示屏
#
# 使用方法：
# 1. 上传到 OpenWrt: scp vfd_monitor.sh root@192.168.1.1:/root/
# 2. 添加执行权限: chmod +x /root/vfd_monitor.sh
# 3. 后台运行: /root/vfd_monitor.sh &
# 4. 开机自启: 添加到 /etc/rc.local
#

# ==================== 配置区 ====================
MQTT_BROKER="192.168.8.3"           # MQTT服务器地址
MQTT_PORT="1883"                     # MQTT端口
MQTT_TOPIC="/espVfd/message"         # MQTT主题
MQTT_USER=""                         # MQTT用户名（如果需要）
MQTT_PASS=""                         # MQTT密码（如果需要）
PING_TARGET="202.96.209.5"           # Ping目标地址（电信DNS）
UPDATE_INTERVAL=1                    # 更新间隔（秒）
# ===============================================

# 颜色输出
log_info() {
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] INFO: $1"
}

log_error() {
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] ERROR: $1" >&2
}

# 检查mosquitto_pub是否安装
check_dependencies() {
    if ! command -v mosquitto_pub >/dev/null 2>&1; then
        log_error "mosquitto_pub not found. Please install: opkg install mosquitto-client-ssl"
        exit 1
    fi
    
    if ! command -v ping >/dev/null 2>&1; then
        log_error "ping not found"
        exit 1
    fi
    
    log_info "Dependencies check passed"
}

# 获取当前时间
get_current_time() {
    date '+%H:%M:%S'
}

# 获取当前日期
get_current_date() {
    date '+%Y-%m-%d'
}

# 获取剩余内存（MB）
get_memory_free() {
    # 读取 /proc/meminfo 获取可用内存
    local mem_free=$(awk '/MemFree:/ {print int($2/1024)}' /proc/meminfo 2>/dev/null)
    local mem_available=$(awk '/MemAvailable:/ {print int($2/1024)}' /proc/meminfo 2>/dev/null)
    
    # 优先使用 MemAvailable，否则使用 MemFree
    if [ -n "$mem_available" ] && [ "$mem_available" -gt 0 ]; then
        echo "${mem_available}MB"
    elif [ -n "$mem_free" ]; then
        echo "${mem_free}MB"
    else
        echo "N/A"
    fi
}

# 获取CPU使用率（百分比）
get_cpu_usage() {
    # 使用 top 命令获取 CPU 使用率
    # OpenWrt 的 top 命令输出格式：CPU:  5% usr  2% sys  0% nic 93% idle
    local cpu_line=$(top -bn1 | grep '^CPU:' 2>/dev/null | head -1)
    
    if [ -n "$cpu_line" ]; then
        # 提取 idle 百分比（更精确的方法）
        local idle=$(echo "$cpu_line" | awk '{for(i=1;i<=NF;i++) if($(i+1)=="idle") print $i}' | tr -d '%')
        
        if [ -n "$idle" ] && [ "$idle" -ge 0 ] 2>/dev/null; then
            local usage=$((100 - idle))
            echo "${usage}%"
        else
            echo "N/A"
        fi
    else
        # 备选方案：读取 /proc/stat
        # 注意：这个方法需要两次采样，但我们用单次采样估算
        local cpu_info=$(awk '/^cpu / {user=$2; nice=$3; system=$4; idle=$5; total=user+nice+system+idle; if(total>0) print int((total-idle)*100/total); else print 0}' /proc/stat)
        if [ -n "$cpu_info" ]; then
            echo "${cpu_info}%"
        else
            echo "N/A"
        fi
    fi
}

# 测量ping延迟（毫秒）
get_ping_latency() {
    local target=$1
    
    # 发送1个ping包，超时1秒
    local ping_result=$(ping -c 1 -W 1 "$target" 2>/dev/null | grep 'time=' | sed 's/.*time=\([0-9.]*\).*/\1/')
    
    if [ -z "$ping_result" ]; then
        echo "Timeout"
    else
        # 格式化延迟，保留一位小数
        printf "%.1fms" "$ping_result"
    fi
}

# 发送MQTT消息
send_mqtt_message() {
    local message="$1"
    
    # 直接调用 mosquitto_pub（不使用 eval，避免空格被压缩）
    if [ -n "$MQTT_USER" ] && [ -n "$MQTT_PASS" ]; then
        mosquitto_pub -h "$MQTT_BROKER" -p "$MQTT_PORT" -t "$MQTT_TOPIC" -u "$MQTT_USER" -P "$MQTT_PASS" -m "$message" 2>/dev/null
    elif [ -n "$MQTT_USER" ]; then
        mosquitto_pub -h "$MQTT_BROKER" -p "$MQTT_PORT" -t "$MQTT_TOPIC" -u "$MQTT_USER" -m "$message" 2>/dev/null
    else
        mosquitto_pub -h "$MQTT_BROKER" -p "$MQTT_PORT" -t "$MQTT_TOPIC" -m "$message" 2>/dev/null
    fi
    
    if [ $? -eq 0 ]; then
        return 0
    else
        log_error "Failed to send MQTT message"
        return 1
    fi
}

# 主循环
main_loop() {
    log_info "Starting VFD Monitor..."
    log_info "MQTT Broker: $MQTT_BROKER:$MQTT_PORT"
    log_info "MQTT Topic: $MQTT_TOPIC"
    log_info "Ping Target: $PING_TARGET"
    log_info "Update Interval: ${UPDATE_INTERVAL}s"
    log_info "Press Ctrl+C to stop"
    echo ""
    
    local loop_count=0
    
    while true; do
        # 获取当前时间
        local current_time=$(get_current_time)
        
        # 获取ping延迟
        local ping_latency=$(get_ping_latency "$PING_TARGET")
        
        # 获取内存和CPU（可选：轮流显示以减少开销）
        local mem_free=$(get_memory_free)
        local cpu_usage=$(get_cpu_usage)
        
        # 构建消息（两行显示）
        # 第一行：时间 + Ping（中间空两格）
        # 第二行：内存 + CPU
        local message="-${current_time}- -${ping_latency}-|Mem:${mem_free} CPU:${cpu_usage}"
        
        # 发送到MQTT
        if send_mqtt_message "$message"; then
            loop_count=$((loop_count + 1))
            printf "\r[%04d] %s | %s | %s | %s      " "$loop_count" "$current_time" "$ping_latency" "$mem_free" "$cpu_usage"
        else
            printf "\r[%04d] %s | Failed to send" "$loop_count" "$current_time"
        fi
        
        # 等待指定间隔
        sleep $UPDATE_INTERVAL
    done
}

# 信号处理：优雅退出
cleanup() {
    echo ""
    log_info "Received stop signal, exiting..."
    exit 0
}

# 捕获中断信号
trap cleanup INT TERM

# 主程序入口
log_info "VFD Display Monitor starting..."

# 检查依赖
check_dependencies

# 启动主循环
main_loop
