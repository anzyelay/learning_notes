#!/bin/bash
# 智能网关脚本 v3.0 —— 策略路由 + 带宽限制 + 故障切换
# 作者：Qwen

set -e

# ================== 配置 ==================
# 内网接口
declare -A LAN_IFS=(
    ["eth0"]="192.168.225.0/24"
    ["ap0"]="192.168.46.0/24"
)
# 外网接口
WAN_PRIMARY="wlan0"      # 主出口
WAN_BACKUP="eth1"        # 备用出口
# 客户端与wlan0同网段，在wlan0所联接的网络下, 服务器在内网eth0下面
# 此处的需要就是让外网的客户端通过指定端口参访问到内网的服务器
CLIENT_NET="10.58.56.0/23"
SERVER_IP="192.168.225.10"
FORWARD_PORTS="13400,3496"

CHECK_HOST="8.8.8.8"     # 用于检测外网连通性
CHECK_INTERVAL=5         # 秒
SCRIPT_NAME="/usr/local/bin/gateway-failover.sh"

# ==========================================

echo ">>> 启用 IPv4 转发与优化..."
echo 1 > /proc/sys/net/ipv4/ip_forward
sysctl -w net.ipv4.conf.all.rp_filter=2
sysctl -w net.netfilter.nf_conntrack_max=262144
sysctl -w net.bridge.bridge-nf-call-iptables=0 2>/dev/null || true

iptables -P FORWARD DROP

rule_exists() {
    local table="$1"; shift
    local chain="$1"; shift
    iptables -t "$table" -C "$chain" "$@" &>/dev/null
}

# =============== 1. NAT 规则 ===============
# MASQUERADE 的工作原理
# MASQUERADE 是一种动态的 SNAT（源地址转换）。
# 它会在数据包发出时，自动使用出站接口（这里是 $wan）的当前 IP 地址作为源地址进行伪装。
# 它适用于IP 可能变化的场景（如 DHCP、PPPoE 拨号等），因为它不需要预先指定 IP
# 如果 WAN 是静态 IP，推荐使用 SNAT`iptables -t nat -A POSTROUTING -o "$wan" -j SNAT --to-source 203.0.113.42`
echo ">>> 配置 MASQUERADE..."
for wan in "$WAN_PRIMARY" "$WAN_BACKUP"; do
    if ! rule_exists nat POSTROUTING -o "$wan" -j MASQUERADE; then
        iptables -t nat -A POSTROUTING -o "$wan" -j MASQUERADE
    fi
done

# =============== 2. 端口映射 ===============
# 入流目标地从外网映射到内网服务器
echo ">>> 配置 DNAT..."
if ! rule_exists nat PREROUTING -i "$WAN_PRIMARY" -d "$CLIENT_NET" -p tcp -m multiport --dports "$FORWARD_PORTS" -j DNAT --to "$SERVER_IP"; then
    iptables -t nat -A PREROUTING -i "$WAN_PRIMARY" -d "$CLIENT_NET" -p tcp -m multiport --dports "$FORWARD_PORTS" -j DNAT --to "$SERVER_IP"
fi

# =============== 3. FORWARD 规则 ===============
echo ">>> 配置 FORWARD..."

# 内网上网
for net in "${LAN_IFS[@]}"; do
    iptables -A FORWARD -s "$net" -j ACCEPT 2>/dev/null || true
done

# 内网互通
iptables -A FORWARD -i eth0 -o ap0 -j ACCEPT 2>/dev/null || true
iptables -A FORWARD -i ap0 -o eth0 -j ACCEPT 2>/dev/null || true

# 端口映射入站
iptables -A FORWARD -i "$WAN_PRIMARY" -o eth0 -d "$SERVER_IP" -p tcp -m multiport --dports "$FORWARD_PORTS" -m conntrack --ctstate NEW,ESTABLISHED,RELATED -j ACCEPT 2>/dev/null || true

# 回程
iptables -A FORWARD -m conntrack --ctstate ESTABLISHED,RELATED -j ACCEPT 2>/dev/null || true

# =============== 4. 策略路由初始化 ===============
echo ">>> 配置策略路由..."

# 获取主/备接口的网关（假设已通过 DHCP 或静态配置）
PRIMARY_GW=$(ip route show dev "$WAN_PRIMARY" | grep -m1 'default via' | awk '{print $3}')
BACKUP_GW=$(ip route show dev "$WAN_BACKUP" | grep -m1 'default via' | awk '{print $3}')

if [ -z "$PRIMARY_GW" ]; then
    echo "⚠️ 未检测到 $WAN_PRIMARY 的默认网关，尝试从路由表提取..."
    PRIMARY_GW=$(ip route show default | grep "$WAN_PRIMARY" | awk '{print $3}' | head -1)
fi
if [ -z "$BACKUP_GW" ]; then
    BACKUP_GW=$(ip route show default | grep "$WAN_BACKUP" | awk '{print $3}' | head -1)
fi

if [ -z "$PRIMARY_GW" ] || [ -z "$BACKUP_GW" ]; then
    echo "❌ 无法获取主备网关，请确保网络已连接！"
    exit 1
fi

echo "主网关 ($WAN_PRIMARY): $PRIMARY_GW"
echo "备网关 ($WAN_BACKUP): $BACKUP_GW"

# 创建备用路由表
echo "100 backup" >> /etc/iproute2/rt_tables 2>/dev/null || true

# 清理旧规则
ip rule del fwmark 1 table main 2>/dev/null || true
ip rule del fwmark 2 table backup 2>/dev/null || true
ip route flush table backup 2>/dev/null || true

# 添加备用路由
ip route add default via "$BACKUP_GW" dev "$WAN_BACKUP" table backup

# 所有流量默认走 main（即主出口），无需额外策略（因为 main 表已有默认路由）

# =============== 5. 带宽限制 (100Mbps) ===============
echo ">>> 配置 100Mbps 出口限速..."

setup_tc() {
    local dev=$1
    # 删除旧 qdisc
    tc qdisc del dev "$dev" root 2>/dev/null || true
    # 添加 HTB 根队列，限速 100Mbps
    tc qdisc add dev "$dev" root handle 1: htb default 10
    tc class add dev "$dev" parent 1: classid 1:1 htb rate 100mbit ceil 100mbit
    tc class add dev "$dev" parent 1:1 classid 1:10 htb rate 100mbit ceil 100mbit
    echo "  - $dev 限速 100Mbps"
}

setup_tc "$WAN_PRIMARY"
setup_tc "$WAN_BACKUP"

# =============== 6. 故障切换守护脚本 ===============
echo ">>> 安装故障切换守护进程..."

cat > "$SCRIPT_NAME" << EOF
#!/bin/bash
PRIMARY="$WAN_PRIMARY"
BACKUP="$WAN_BACKUP"
CHECK_HOST="$CHECK_HOST"
PRIMARY_GW="$PRIMARY_GW"
BACKUP_GW="$BACKUP_GW"

# 当前状态：0=主, 1=备
STATE_FILE="/tmp/gateway.state"
[ -f "\$STATE_FILE" ] && CURRENT=\$(cat \$STATE_FILE) || CURRENT=0

check_primary() {
    timeout 3 ping -I \$PRIMARY -c 1 -W 2 \$CHECK_HOST >/dev/null 2>&1
}

switch_to_backup() {
    if [ "\$CURRENT" -eq 0 ]; then
        echo "🔄 切换到备用链路 (\$BACKUP)..."
        ip route replace default via \$BACKUP_GW dev \$BACKUP
        echo 1 > \$STATE_FILE
        CURRENT=1
    fi
}

switch_to_primary() {
    if [ "\$CURRENT" -eq 1 ]; then
        echo "🔄 切换回主链路 (\$PRIMARY)..."
        ip route replace default via \$PRIMARY_GW dev \$PRIMARY
        echo 0 > \$STATE_FILE
        CURRENT=0
    fi
}

while true; do
    if check_primary; then
        switch_to_primary
    else
        switch_to_backup
    fi
    sleep $CHECK_INTERVAL
done
EOF

chmod +x "$SCRIPT_NAME"

# 启动守护进程（后台）
if ! pgrep -f "$(basename $SCRIPT_NAME)" > /dev/null; then
    nohup "$SCRIPT_NAME" > /var/log/gateway-failover.log 2>&1 &
    echo "守护进程已启动: $(pgrep -f $(basename $SCRIPT_NAME))"
fi

# ==========================================
echo ""
echo "✅ 智能网关配置完成！"
echo "  • 主出口: $WAN_PRIMARY (优先)"
echo "  • 备用出口: $WAN_BACKUP (故障切换)"
echo "  • 内网总出口带宽限制: 100Mbps/接口"
echo "  • 故障检测日志: /var/log/gateway-failover.log"
echo ""
echo "💡 持久化建议："
echo "  1. 保存 iptables: iptables-save > /etc/iptables/rules.v4"
echo "  2. 将本脚本加入开机启动（如 systemd 或 rc.local）"
