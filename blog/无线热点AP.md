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
dnsmasq --conf-file=/data/dnsmasq.conf --dhcp-leasefile=/data/dnsmasq.leases --addn-hosts=/data/hosts --pid-file=/data/dnsmasq.pid -i bridge0 -I lo -z --dhcp-range=bridge0,192.168.10.9,192.168.10.100,255.255.255.0,86400 --dhcp-hostsfile=/data/dhcp_hosts --dhcp-option-force=6,192.168.10.1 --dhcp-option-force=26,1358 --dhcp-option-force=120,abcd.com --dhcp-script=/bin/dnsmasq_script.sh
```

## wpa_supplicant

### ARCHITECTURE

1. wpa_supplicant.conf: 描述用户想要连接的热点接口的配置文件
2. wpa_supplicant： 与网络接口通信的程序
3. wpa_cli： 为wpa_supplicant提供高级接口的客户端
4. wpa_passphrase： 构建包含加密密码的 wpa_supplicant.conf 文件所需的实用程序

## iwpriv

> iwpriv - configure optionals (private) parameters of a wireless network interface

与wifi driver找交道的工具，通过IOCTL的实现来设置、获取驱动参数。
