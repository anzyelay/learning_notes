# 无线热点AP

1. [Linux Wireless](https://wireless.wiki.kernel.org/en/users/documentation)

1. [wpa_supplicant / hostapd](https://w1.fi/wpa_supplicant/devel/)

## architecture

module | function
-|-
dnsmasq | DHCP/DNS服务
wpa_supplicant | 在客户端运行的热点访问程序
hostapd | 在服务端运行的热点认证管理服务
iw | iw is a new nl80211 based CLI configuration utility for wireless devices

- cfg80211 : new driver configuration API --> linux driver layer
- nl80211 : new userspace <–> kernelspace wireless driver communication transport

## hostapd

### what is hostapd ?

运行在用户空间的热点连接与认证管理服务的守护进程

> NAME
> hostapd - IEEE 802.11 AP, IEEE 802.1X/WPA/WPA2/EAP/RADIUS Authenticator
>
> SYNOPSIS
> hostapd [-hdBKtv] [-P \<PID file\>] <configuration file(s)>
>
> DESCRIPTION
> This manual page documents briefly the hostapd daemon.
>
> hostapd  is a user space daemon for access point and authentication servers.  It implements IEEE 802.11 access point management, IEEE 802.1X/WPA/WPA2/EAP Authenticators and RADIUS authentication server.  The cur‐
> rent version supports Linux (Host AP, mac80211-based drivers) and FreeBSD (net80211).
>
> hostapd is designed to be a "daemon" program that runs in the background and acts as the backend component controlling authentication.  hostapd supports separate frontend programs and an example text-based front end, hostapd_cli, is included with hostapd.

```sh
# -B : configure file
# -P : pid file
# -e : entropy file
hostapd -B /data/misc/wifi/hostapd.conf -P /data/hostapd_ssid1.pid -e /data/misc/wifi/entropy_file
```

### hostapd_cli 命令

```bash
# 显示associate clients, （更改密码后，自动连接未认证通过的也会显示在此，需要disassociate下？）
hostapd_cli -i interface list_sta
# 显示某个sta的信息
hostapd_cli -i interface sta <MAC addr>
# 显示所有sta信息
hostapd_cli -i interface all_sta
# disassociate sta
hostapd_cli -i interface disassociate <MAC addr>
# deauthenticate
hostapd_cli -i interface deauthen <MAC addr>

```

运行脚本

```sh
# -B：后台运行 
# -a：hostapd发生事件时运行的脚本, eg: /usr/bin/QCMAP_StaInterface
# -p: control sockets的寻找路径
hostapd_cli -i wlan0 -p /var/run/hostapd -B -a /usr/bin/QCMAP_StaInterface
```

## dnsmasq

> A lightweight DHCP and caching DNS server.

```sh
# --dhcp-script: event 时的运行脚本
# --dhcp-hostsfile：保留IP地址文件,每行与--dhcp-host规则的"="右边的文本一样,与之不同
# 之处在于，更改文件后，不需要重启进程，发送信号SIGHUP即可生效
# --addn-hosts: 附加hosts文件，规则与/etc/hosts一样
# --log-facility: 日志文件
dnsmasq --conf-file=/data/dnsmasq.conf --dhcp-leasefile=/data/dnsmasq.leases --addn-hosts=/data/hosts --pid-file=/data/dnsmasq.pid -i bridge0 -I lo -z --dhcp-range=bridge0,192.168.10.9,192.168.10.100,255.255.255.0,86400 --dhcp-hostsfile=/data/dhcp_hosts --dhcp-option-force=6,192.168.10.1 --dhcp-option-force=26,1358 --dhcp-option-force=120,abcd.com --dhcp-script=/bin/dnsmasq_script.sh
```

NOTE:

> When it receives a **SIGHUP**, dnsmasq clears its cache and then re-loads /etc/hosts and /etc/ethers and any file given by --dhcp-hostsfile, --dhcp-hostsdir, --dhcp-optsfile,  --dhcp-optsdir,  --addn-
> hosts  or  --hostsdir.   The  DHCP lease change script is called for all existing DHCP leases. If --no-poll is set SIGHUP also re-reads /etc/resolv.conf.  SIGHUP does NOT re-read the configuration
> file.

### leasefile

使用`--dhcp-leasefile`指定leasefile文件。

file format as follow:

```txt
expire  mac ip hostname(无为空或*) clid(无为*)
1693468131 a4:cf:99:73:5d:d0 192.168.10.31 JBD-Vehicle-1-SW-MacBookPro 01:a4:cf:99:73:5d:d0
1693468074 6e:f3:73:94:87:c3 192.168.10.30 F1339899-PC-1 01:6e:f3:73:94:87:c3
```

- expire: 为期望的租赁到期时间，如果时间走到该时刻存在两种情况：1）当节点不存在时，则释放占用的地址, 并发出del信号；2）当节点存在，则续租，更新期望时间。
- 查看`lease_update_file(time_t now)`函数可知， 更新expire时，是清空原先内容，全部lease结点重写一遍。 故在程序运行中时，外部更改文件下次更新文件时又被还原。
- 重启dnsmasq时，程序先读取该文件的信息以重新加入上次退出时的缓冲的结点信息。

### dhcp-script

> Whenever a new DHCP lease is created, or an old one destroyed, or a TFTP file transfer completes, the executable specified by this option is run.  <path> must be an  absolute  pathname,  no
> PATH search occurs.  The arguments to the process are "add", "old" or "del", the MAC address of the host (or DUID for IPv6) , the IP address, and the hostname, if known. "add" means a lease
> has been created, "del" means it has been destroyed, "old" is a notification of an existing lease when dnsmasq starts or a change to MAC address or hostname  of  an  existing  lease  (also,
> lease  length or expiry and client-id, if --leasefile-ro is set and lease expiry if --script-on-renewal is set).  If the MAC address is from a network type other than ethernet, it will have
> the network type prepended, eg "06-01:23:45:67:89:ab" for token ring. The process is run as root (assuming that dnsmasq was originally run as root) even if dnsmasq is configured  to  change
> UID to an unprivileged user.

- 无论何时，只要有DHCP节点被创建，销毁或者tftp文件传输完成， 指定的脚本就会运行，且路径必须是绝对路径。

- 传入的参数： event mac ip [hostname](可能没有)

- event有如下三种

  - add: 有新节点创建则发生

  - del: 结点被销毁则发生（结点离开超过expire time后则被销毁, eg:可以更改dnsmasq.lease中的时间记录然后重启dnsmasq演示该情况. 有新连接时会判断旧的结点情况并更新leases文件）

  - old: 已存在的结点(在dnsmasq.lease)在重启dnsmasq时，mac或hostname变更

  - 如果加了参数`--leasefile-ro`和`--script-on-renewal`，则结点期望时间和clid更新也会发出事件消息，可以参考dnsmasq工程中的contrib/wrt/lease_update.sh。

注意：

> At dnsmasq startup, the script will be invoked for all existing leases as they are read from the lease file. Expired leases will be called with "del" and others with "old". When dnsmasq re‐
> ceives a HUP signal, the script will be invoked for existing leases with an "old" event.

- lease file只是为重启dnsmasq时可以读取上次的lease信息做为启动的初始运行参数，但在运行中时，当lease file被外部更改后，dnsmasq在另一结点的期望时间到时会去同步该文件与自身保持一致。

## wpa_supplicant

### ARCHITECTURE

1. wpa_supplicant.conf: 描述用户想要连接的热点接口的配置文件
2. wpa_supplicant： 与网络接口通信的程序
3. wpa_cli： 为wpa_supplicant提供高级接口的客户端
4. wpa_passphrase： 构建包含加密密码的 wpa_supplicant.conf 文件所需的实用程序

## iwpriv

> iwpriv - configure optionals (private) parameters of a wireless network interface

与wifi driver找交道的工具，通过IOCTL的实现来设置、获取驱动参数。
