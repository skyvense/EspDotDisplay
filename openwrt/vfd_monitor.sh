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
PING_TARGET="202.96.209.5"           # Line 1 Ping目标地址（电信DNS）

# Line 2 监控主机列表 (格式: "Host1 Host2 Host3 ...")
MONITOR_HOSTS="lisa.ddn.pw us.ddn.pw dm.ddn.pw"
# Line 2 显示名称列表 (格式: "Name1 Name2 Name3 ...", 需与主机一一对应)
MONITOR_NAMES="lisa us dm"

UPDATE_INTERVAL=0.2                  # 更新间隔（秒） - 5Hz刷新率
DATA_FILE="/tmp/vfd_monitor.state"   # 数据共享文件
PID_FILE="/tmp/vfd_monitor.pid"      # 后台Ping进程PID
# ===============================================

# 颜色输出
log_info() {
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] INFO: $1"
}

log_error() {
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] ERROR: $1" >&2
}

# 检查依赖
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

# 测量ping延迟（毫秒）
get_ping_latency() {
    local target=$1
    # 发送1个ping包，超时1秒
    local ping_result=$(ping -c 1 -W 1 "$target" 2>/dev/null | grep 'time=' | sed 's/.*time=\([0-9.]*\).*/\1/')
    
    if [ -z "$ping_result" ]; then
        echo "Timeout"
    else
        printf "%.1fms" "$ping_result"
    fi
}

# 发送MQTT消息
send_mqtt_message() {
    local message="$1"
    if [ -n "$MQTT_USER" ] && [ -n "$MQTT_PASS" ]; then
        mosquitto_pub -h "$MQTT_BROKER" -p "$MQTT_PORT" -t "$MQTT_TOPIC" -u "$MQTT_USER" -P "$MQTT_PASS" -m "$message" 2>/dev/null
    elif [ -n "$MQTT_USER" ]; then
        mosquitto_pub -h "$MQTT_BROKER" -p "$MQTT_PORT" -t "$MQTT_TOPIC" -u "$MQTT_USER" -m "$message" 2>/dev/null
    else
        mosquitto_pub -h "$MQTT_BROKER" -p "$MQTT_PORT" -t "$MQTT_TOPIC" -m "$message" 2>/dev/null
    fi
    return $?
}

# 后台Ping守护进程
ping_daemon() {
    log_info "Ping daemon started"
    
    # 预处理主机数
    local host_count=$(echo "$MONITOR_HOSTS" | wc -w | tr -d ' ')
    
    while true; do
        # 1. Ping 主目标
        local p_main=$(get_ping_latency "$PING_TARGET")
        
        # 2. Ping 监控列表
        local formatted=""
        local i=1
        
        # 遍历所有主机
        for host in $MONITOR_HOSTS; do
            local lat=$(get_ping_latency "$host")
            local name=$(echo "$MONITOR_NAMES" | cut -d' ' -f$i)
            
            formatted="$formatted$name:$lat "
            i=$((i + 1))
        done
        
        # 原子写入数据文件
        local tmp_file="${DATA_FILE}.tmp"
        echo "PING_MAIN='$p_main'" > "$tmp_file"
        echo "LINE2_TEXT='$formatted'" >> "$tmp_file"
        mv "$tmp_file" "$DATA_FILE"
        
        # 休息一下（数据更新频率不需要和显示刷新率一样高）
        sleep 1
    done
}

start_daemon() {
    ping_daemon &
    echo $! > "$PID_FILE"
}

stop_daemon() {
    if [ -f "$PID_FILE" ]; then
        local pid=$(cat "$PID_FILE")
        if [ -n "$pid" ] && kill -0 "$pid" 2>/dev/null; then
            kill "$pid" 2>/dev/null
            log_info "Ping daemon stopped"
        fi
        rm "$PID_FILE"
    fi
    # 清理遗留的数据文件
    rm -f "$DATA_FILE"
}

cleanup() {
    stop_daemon
    echo ""
    log_info "Exiting..."
    exit 0
}

trap cleanup INT TERM

# 主循环
main_loop() {
    log_info "Starting VFD Monitor (High Refresh Rate)..."
    log_info "MQTT Broker: $MQTT_BROKER:$MQTT_PORT"
    log_info "Update Interval: ${UPDATE_INTERVAL}s (5Hz)"
    
    # 启动后台Ping进程
    start_daemon
    
    # 等待初始数据
    log_info "Waiting for initial data..."
    while [ ! -f "$DATA_FILE" ]; do
        sleep 0.5
    done
    
    local loop_count=0
    local ping_main="-"
    local line2_text="Loading..."
    
    while true; do
        # 获取当前时间
        local current_time=$(get_current_time)
        
        # 读取最新数据（如果文件存在）
        if [ -f "$DATA_FILE" ]; then
            # 使用 . 命令source文件，加载变量 PING_MAIN 和 LINE2_TEXT
            . "$DATA_FILE"
        fi
        
        # 滚动显示逻辑 (Marquee)
        # 拼接自身以实现循环滚动效果
        local scroll_source="$LINE2_TEXT   $LINE2_TEXT"
        local source_len=$((${#LINE2_TEXT} + 3))
        
        # 防止空字符串导致的错误
        if [ "$source_len" -le 3 ]; then
            source_len=10
            scroll_source="Loading...   Loading..."
        fi
        
        local scroll_pos=$((loop_count % source_len))
        
        # 截取20个字符 (VFD宽度)
        local line2_display=${scroll_source:$scroll_pos:20}
        
        # 兼容性补全（如果截取长度不足20）
        if [ ${#line2_display} -lt 20 ]; then
             # 尝试用cut补救，或者直接补空格
             local remaining=$((20 - ${#line2_display}))
             # 简单的补空格策略
             while [ ${#line2_display} -lt 20 ]; do
                line2_display="${line2_display} "
             done
        fi
        
        # 构建最终消息
        local message="-${current_time}- -${PING_MAIN}-|${line2_display}"
        
        # 发送到MQTT
        if send_mqtt_message "$message"; then
            # 仅在每5次循环（约1秒）打印一次日志，避免刷屏
            if [ $((loop_count % 5)) -eq 0 ]; then
                printf "\r[%04d] %s | %s      " "$loop_count" "$current_time" "$line2_display"
            fi
        else
            printf "\r[%04d] %s | Failed to send" "$loop_count" "$current_time"
        fi
        
        loop_count=$((loop_count + 1))
        sleep $UPDATE_INTERVAL
    done
}

# 主程序入口
log_info "VFD Display Monitor starting..."
check_dependencies
main_loop
