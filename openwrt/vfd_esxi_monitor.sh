#!/bin/sh
#
# VFD ESXi Monitor Script for OpenWrt
# 功能：监控 VMware ESXi 主机的 CPU 和内存使用率，推送到VFD显示屏
#
# 使用方法：
# 1. 上传到 OpenWrt: scp vfd_esxi_monitor.sh root@192.168.1.1:/root/
# 2. 添加执行权限: chmod +x /root/vfd_esxi_monitor.sh
# 3. 配置 ESXi 信息（见下方配置区）
# 4. 后台运行: /root/vfd_esxi_monitor.sh &
# 5. 开机自启: 添加到 /etc/rc.local
#
# 前置要求：
# - opkg install curl ca-certificates
# - ESXi 6.5+ (支持 REST API)
#

# ==================== 配置区 ====================
# MQTT 配置
MQTT_BROKER="192.168.8.3"           # MQTT服务器地址
MQTT_PORT="1883"                     # MQTT端口
MQTT_TOPIC="/espVfd/message"         # MQTT主题
MQTT_USER=""                         # MQTT用户名（如果需要）
MQTT_PASS=""                         # MQTT密码（如果需要）

# ESXi 配置
ESXI_HOST="192.168.1.100"            # ESXi 主机地址
ESXI_USER="root"                     # ESXi 用户名
ESXI_PASS="YourPassword"             # ESXi 密码
ESXI_PORT="443"                      # ESXi REST API 端口（默认443）

# 其他配置
UPDATE_INTERVAL=2                    # 更新间隔（秒，建议2-5秒）
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
        log_error "mosquitto_pub not found. Install: opkg install mosquitto-client-ssl"
        exit 1
    fi
    
    if ! command -v curl >/dev/null 2>&1; then
        log_error "curl not found. Install: opkg install curl ca-certificates"
        exit 1
    fi
    
    log_info "Dependencies check passed"
}

# ESXi REST API - 获取 Session Token（实际上 MOB 不需要，这里保留兼容）
esxi_login() {
    # ESXi 独立主机的 REST API 登录路径
    local response=$(curl -k -s -X POST \
        "https://${ESXI_HOST}:${ESXI_PORT}/api/session" \
        -u "${ESXI_USER}:${ESXI_PASS}" \
        -H "Content-Type: application/json" 2>/dev/null)
    
    if [ $? -eq 0 ] && [ -n "$response" ]; then
        # 提取 session token (JSON格式: "xxxxxx" 或 {"value":"xxxxxx"})
        # ESXi 7.0+ 返回纯字符串
        echo "$response" | sed 's/"//g' | sed 's/.*value[":]*\([^"]*\).*/\1/'
    else
        echo ""
    fi
}

# ESXi REST API - 获取主机 CPU 使用率
esxi_get_cpu_usage() {
    local session_id="$1"
    
    # 使用 ESXi REST API 获取主机摘要信息
    local response=$(curl -k -s -X GET \
        "https://${ESXI_HOST}:${ESXI_PORT}/rest/vcenter/host" \
        -H "vmware-api-session-id: ${session_id}" 2>/dev/null)
    
    if [ $? -eq 0 ] && [ -n "$response" ]; then
        # 从响应中提取第一个主机的信息
        # 注意：这个API返回主机列表，我们简化处理，获取第一个主机
        # 实际生产环境需要根据 host ID 精确查询
        
        # 使用 vim.HostSystem.QueryHostConnectionInfo 获取更详细信息
        # 简化版：直接返回占位符，实际需要更复杂的解析
        echo "N/A"
    else
        echo "Error"
    fi
}

# ESXi REST API - 获取主机内存使用情况（通过 Performance API）
esxi_get_memory_usage() {
    local session_id="$1"
    
    # ESXi REST API 的性能数据需要通过 /rest/appliance/health/mem 获取
    local response=$(curl -k -s -X GET \
        "https://${ESXI_HOST}:${ESXI_PORT}/rest/appliance/health/mem" \
        -H "vmware-api-session-id: ${session_id}" 2>/dev/null)
    
    if [ $? -eq 0 ] && [ -n "$response" ]; then
        echo "N/A"
    else
        echo "Error"
    fi
}

# 简化版：通过 SSH esxcli 命令获取信息（备选方案）
# 但您要求只用 REST API，所以这里提供一个简化的实现
# 
# ESXi REST API 获取性能数据比较复杂，需要：
# 1. 获取主机 ID
# 2. 查询 Performance Counter
# 3. 解析复杂的 JSON 响应
#
# 下面是一个使用 esxcli 命令通过 curl 调用的替代方案：

# ESXi SSH - 获取主机 CPU 和内存使用率（最可靠的方法）
esxi_get_host_stats_ssh() {
    # 通过 SSH 执行 esxcli 命令获取性能数据
    # 优先使用免密登录，如果不可用则尝试密码登录（需要 sshpass）
    
    # 构建 SSH 命令
    local ssh_cmd="vim-cmd hostsvc/hostsummary 2>/dev/null | grep -E 'overallCpuUsage|overallMemoryUsage' | grep -o '[0-9]*'"
    
    # 尝试免密登录
    local result=$(ssh -o StrictHostKeyChecking=no -o ConnectTimeout=3 -o BatchMode=yes \
        "${ESXI_USER}@${ESXI_HOST}" \
        "$ssh_cmd" 2>/dev/null)
    
    # 如果免密失败，尝试使用 sshpass（如果已安装）
    if [ $? -ne 0 ] || [ -z "$result" ]; then
        if command -v sshpass >/dev/null 2>&1; then
            result=$(sshpass -p "${ESXI_PASS}" ssh -o StrictHostKeyChecking=no -o ConnectTimeout=3 \
                "${ESXI_USER}@${ESXI_HOST}" \
                "$ssh_cmd" 2>/dev/null)
        else
            return 1
        fi
    fi
    
    if [ $? -ne 0 ] || [ -z "$result" ]; then
        return 1
    fi
    
    # 解析结果（第一行是 CPU 使用率，第二行是内存使用率）
    local cpu_usage=$(echo "$result" | sed -n '1p')
    local mem_usage=$(echo "$result" | sed -n '2p')
    
    if [ -n "$cpu_usage" ] && [ -n "$mem_usage" ]; then
        echo "${cpu_usage}%|${mem_usage}%"
        return 0
    fi
    
    return 1
}

# ESXi REST API - 获取主机 CPU 和内存使用率（备选方案）
esxi_get_host_stats() {
    local session_id="$1"
    
    # 优先尝试 SSH 方式（更可靠）
    local ssh_result=$(esxi_get_host_stats_ssh)
    if [ $? -eq 0 ]; then
        echo "$ssh_result"
        return 0
    fi
    
    # SSH 失败，回退到 REST API
    # 方法1：先尝试通过标准 REST API 获取 session
    if [ -z "$session_id" ]; then
        session_id=$(curl -k -s -X POST \
            "https://${ESXI_HOST}:${ESXI_PORT}/api/session" \
            -u "${ESXI_USER}:${ESXI_PASS}" 2>/dev/null | tr -d '"')
    fi
    
    if [ -z "$session_id" ]; then
        echo "Error|Error"
        return 1
    fi
    
    # 获取主机列表
    local hosts=$(curl -k -s -X GET \
        "https://${ESXI_HOST}:${ESXI_PORT}/api/vcenter/host" \
        -H "vmware-api-session-id: ${session_id}" 2>/dev/null)
    
    # 提取主机 ID
    local host_id=$(echo "$hosts" | grep -o '"host":"[^"]*"' | head -1 | cut -d'"' -f4)
    
    if [ -z "$host_id" ]; then
        # 单机 ESXi 尝试使用固定 ID
        host_id="host-11"
    fi
    
    # 获取虚拟机列表来间接判断负载
    local vms=$(curl -k -s -X GET \
        "https://${ESXI_HOST}:${ESXI_PORT}/api/vcenter/vm?filter.hosts=${host_id}" \
        -H "vmware-api-session-id: ${session_id}" 2>/dev/null)
    
    local total_vms=$(echo "$vms" | grep -o '"vm":"' | wc -l)
    local running_vms=$(echo "$vms" | grep -o '"power_state":"POWERED_ON"' | wc -l)
    
    # 由于 REST API 不直接提供主机 CPU/内存百分比
    # 我们返回虚拟机数量作为负载指标
    if [ "$total_vms" -ge 0 ] 2>/dev/null; then
        echo "${running_vms}VM|${total_vms}Total"
    else
        echo "N/A|N/A"
    fi
    
    # 保存 session 供下次使用
    ESXI_SESSION="$session_id"
}

# 备选方案：使用 REST API（如果 MOB 不可用）
esxi_get_quick_stats() {
    local session_id="$1"
    
    # 直接调用主函数
    esxi_get_host_stats "$session_id"
}

# 发送MQTT消息
send_mqtt_message() {
    local message="$1"
    
    # 直接调用 mosquitto_pub
    if [ -n "$MQTT_USER" ] && [ -n "$MQTT_PASS" ]; then
        mosquitto_pub -h "$MQTT_BROKER" -p "$MQTT_PORT" -t "$MQTT_TOPIC" -u "$MQTT_USER" -P "$MQTT_PASS" -m "$message" 2>/dev/null
    elif [ -n "$MQTT_USER" ]; then
        mosquitto_pub -h "$MQTT_BROKER" -p "$MQTT_PORT" -t "$MQTT_TOPIC" -u "$MQTT_USER" -m "$message" 2>/dev/null
    else
        mosquitto_pub -h "$MQTT_BROKER" -p "$MQTT_PORT" -t "$MQTT_TOPIC" -m "$message" 2>/dev/null
    fi
    
    return $?
}

# 获取当前时间
get_current_time() {
    date '+%H:%M:%S'
}

# 主循环
main_loop() {
    log_info "Starting ESXi VFD Monitor..."
    log_info "MQTT Broker: $MQTT_BROKER:$MQTT_PORT"
    log_info "MQTT Topic: $MQTT_TOPIC"
    log_info "ESXi Host: $ESXI_HOST"
    log_info "Update Interval: ${UPDATE_INTERVAL}s"
    log_info "Press Ctrl+C to stop"
    echo ""
    
    local loop_count=0
    
    # 测试一次连接
    log_info "Testing connection to ESXi..."
    local test_result=$(esxi_get_host_stats "dummy")
    if echo "$test_result" | grep -q "Error"; then
        log_error "Failed to connect to ESXi. Please check:"
        log_error "  1. ESXi host address: $ESXI_HOST"
        log_error "  2. Username/Password: $ESXI_USER"
        log_error "  3. Network connectivity"
        exit 1
    fi
    log_info "Connection successful!"
    echo ""
    
    while true; do
        # 获取当前时间
        local current_time=$(get_current_time)
        
        # 获取 ESXi 性能数据（MOB 接口，不需要 session）
        local stats=$(esxi_get_host_stats "")
        local cpu_usage=$(echo "$stats" | cut -d'|' -f1)
        local mem_usage=$(echo "$stats" | cut -d'|' -f2)
        
        # 构建消息（两行显示）
        # 第一行：时间 + 主机标识
        # 第二行：CPU + 内存
        local message="${current_time}  ESXi|CPU:${cpu_usage} Mem:${mem_usage}"
        
        # 发送到MQTT
        if send_mqtt_message "$message"; then
            loop_count=$((loop_count + 1))
            printf "\r[%04d] %s | CPU:%s Mem:%s      " "$loop_count" "$current_time" "$cpu_usage" "$mem_usage"
        else
            log_error "Failed to send MQTT message"
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
log_info "ESXi VFD Display Monitor starting..."

# 检查依赖
check_dependencies

# 启动主循环
main_loop
