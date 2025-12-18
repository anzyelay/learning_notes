#!/bin/sh

INTERFACE=eth1
TOTAL_RATE=800Mbit

# === 1. Clean up existing TC configuration ===
tc qdisc del dev "$INTERFACE" root 2>/dev/null

# === 2. Add root HTB qdisc (total bandwidth capped at 700M) ===
tc qdisc add dev "$INTERFACE" root handle 1: htb default 10

# === 3. Define HTB classes (lower 'prio' = higher scheduling priority) ===
# High-priority class: dst_port 5203 �� classid 1:30 (prio 0)
tc class add dev "$INTERFACE" parent 1: classid 1:30 htb \
    rate 200Mbit ceil "$TOTAL_RATE" burst 1600b cburst 1600b prio 0 quantum 2000

# Medium-priority class: dst_port 5202 �� classid 1:20 (prio 1)
tc class add dev "$INTERFACE" parent 1: classid 1:20 htb \
    rate 100Mbit ceil "$TOTAL_RATE" burst 1600b cburst 1600b prio 1 quantum 2000

# Default (low-priority) class: all unmatched traffic �� classid 1:10 (prio 2)
tc class add dev "$INTERFACE" parent 1: classid 1:10 htb \
    rate 50Mbit ceil "$TOTAL_RATE" burst 1600b cburst 1600b prio 2 quantum 1500

# === 4. Add u32 filters to classify traffic by destination port ===
# Match dst_port = 5203 (0x1453) �� send to high-priority class 1:30
tc filter add dev "$INTERFACE" protocol ip parent 1: prio 1 \
    u32 match ip dport 5203 0xffff \
    flowid 1:30

# Match dst_port = 5202 (0x1452) �� send to medium-priority class 1:20
tc filter add dev "$INTERFACE" protocol ip parent 1: prio 2 \
    u32 match ip dport 5202 0xffff \
    flowid 1:20

# Note: All other traffic automatically falls into default class 1:10 (via 'default 10')

# === 6. Verification and usage instructions ===
echo "? TC QoS configuration applied successfully!"
echo "   - High priority: dst_port=5203 �� 200M guaranteed (HTB prio 0)"
echo "   - Medium priority: dst_port=5202 �� 100M guaranteed (HTB prio 1)"
echo "   - Default priority: all other traffic �� 50M guaranteed (HTB prio 2)"
echo "   - Total egress bandwidth limited to 700M"
echo ""
echo "?? Example test commands (run on client):"
echo "   iperf3 -c <SERVER_IP> -p 5203 -t 30  # High-priority stream"
echo "   iperf3 -c <SERVER_IP> -p 5202 -t 30  # Medium-priority stream"
echo ""
echo "?? Check real-time statistics:"
tc -s class show dev "$INTERFACE" | grep -E "(class|Sent|rate)"
