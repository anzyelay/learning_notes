# network command of linux

## conntrack

### 作用

> command line interface for netfilter connection tracking

### conntrack 输出含义

position |ip:port | description
-|-|-|
1 | protocol | <li>tcp:6 <li>udp:17
2 | unkown |
origin | src | origin src ip
origin | dst | origin dst ip
origin | sport | origin src port
origin | dport | origin dst port
expect | src | expect reply src ip， 一般与origin dst相同
expect | dst | expect reply dst ip， 一般与origin src相同,有NAT转换与转换ip相同
expect | sport | expect reply origin src port， 一般与origin dport相同
expect | dport | expect reply origin dst port， 一般与origin sport相同，有NAT转换，与转换port相同

```sh
/ # conntrack -L
tcp      6 3508 ESTABLISHED src=192.168.10.170 dst=17.57.145.132 sport=49417 dport=5223 src=17.57.145.132 dst=10.228.118.101 sport=5223 dport=49417 [ASSURED] mark=0 use=1
tcp      6 20 LAST_ACK src=192.168.10.170 dst=17.248.221.12 sport=49474 dport=443 src=17.248.221.12 dst=10.228.118.101 sport=443 dport=49474 [ASSURED] mark=0 use=1

```

### 解释

上述显示结果中，每行表示一个连接跟踪项。

每行相同的地址和端口号会出现两次，而且第二次出现的源地址/端口对和目标地址/端口对会与第一次正好相反！

这是因为每个连接跟踪项会先后两次被插入连接状态表。

第一个四元组（源地址、目标地址、源端口、目标端口）记录的是原始方向的连接信息，即发送者发送报文的方向。

第二个四元组则记录的是连接跟踪子系统**期望收到**的对端回复报文的连接信息。

这解决了两个问题：

- 如果报文匹配到一个NAT规则，例如IP地址伪装，相应的**映射信息**会记录在链接跟踪项的**回复方向部分**，并自动应用于**同一条流**的**所有**后续报文。（即NAT修改后的信息可从期望回复方向部分找出来）

- 即使一条流经过了地址或端口的转换，也可以成功在连接状态表中查找到回复报文的四元组信息。

**原始方向的（第一个显示的）四元组信息永远不会改变：它就是发送者发送的连接信息。**

NAT 操作只会修改回复方向（第二个）四元组，因为这是接受者看到的连接信息。

修改第一个四元组没有意义：netfilter 无法控制发起者的连接状态，它只能在收到/转发报文时对其施加影响。

**当一个报文未映射到现有连接表项时，连接跟踪可以为其新建一个表项。**

- 对于 UDP 报文，该操作会自动进行。

- 对于 TCP 报文，连接跟踪可以配置为只有 TCP 报文设置了 SYN 标志位 才新建表项。

 默认情况下，连接跟踪会允许从流的中间报文开始创建，这是为了避免对启用连接跟踪之前就存在的流处理出现问题。

## brctl

> ethernet bridge administration, brctl is used to set up, maintain, and inspect the ethernet bridge configuration in the Linux kernel

- 创建bridge

    1. `brctl addbr <name>`
    1. `brctl delbr <name>`
    1. `brctl show`

- PORTS
普通网络设备只有两个端口， 一进（phy）一出(协议栈)， 但bridge可以有多个端口

    1. `brctl addif <brname> <ifname>`
    2. `brctl delif <brname> <ifname>`
    3. `brctl show  <brname>`

注：加某个ports前需要先`ifconfig <brname> down`关掉网桥才能设置

此命令已经是过时了，使用`ip`操作如下

```sh
# 创建bridge
ip link add [name] br_name type bridge 
ip link set br_name up
# 删除网桥可以用
ip link delete br_name type bridge
ip link del br_name

# 想要添加Interface到网桥上，interface状态必须是Up
ip link set eth0 up
# 添加eth0 interface到网桥上
ip link set eth0 master br_name
# 从网桥解绑eth0
ip link set eth0 nomaster

# 关闭eth0
ip link set eth0 down
```

## arp

> Arp manipulates or displays the kernel's IPv4 network neighbour cache. It can add entries to the table, delete one or display the current content

1. `arp -a`: 查询ip的mac地址

## iptable

flush清空、list查询、zero清数据：

`iptable [-t table] {-F|-L|-Z} [chain [rulenum]] [options...]`

tables有：

- filter
- mangle
- nat
- raw
- security
