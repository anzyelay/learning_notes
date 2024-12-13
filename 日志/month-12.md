# 2024-12

## 12/2

### PHY的电源无法正常打开

1. 使用`gpiodetect`无法发现io_expand和PMIC两路GPIO口
1. 此两路是通过`i2c`在系统启动时配置的，向系统注册GPIO线路
1. 在系统启动后，使用`i2cdump -y 0 0x20`可以查看到`io_expand`的寄存器值，说明i2c已经正常工作
1. `reboot`热启动发现`io_expand`和`PMIC`的GPIO口可以`gpiodetect`检测到。
1. 用示波器测量发现，i2c通路的SCL,SDA数据通路的电压在系统启动时，电压幅值被拉低，导致无法正常工作，启动后电压正常
1. i2c系统在启动后又能正常工作，说明i2c通路是通的，只是启动时，电压被拉低，导致无法正常工作，可能是启动时，GPIO口被拉低，导致电压被拉低
1. 因此i2c通路下挂了`PMIC`,`io_expand`和`CODEC`三个设备，需要检查是哪个设备导致电压被拉低。
1. 断开codec的i2c发现可正常识别到, codec接上i2c后，I2C0路启动时被拉低,
1. 接回codec通路，直接不去控制它, 发现系统启动后一段时间，使用`i2cdetect -y 0`仍扫描异常。I2C通路一直被干扰，导致其它设备无法正常工作。
1. 由此说明是codec的i2c干扰了i2c0的通路，导致i2c0的通路被拉低，导致其它设备无法正常工作。

## 12/6

### linux下eth0，eth1转发设置

1. `sysctl -w net.ipv4.ip_forward=1` 开启ip转发， 实际上就是修改`/proc/sys/net/ipv4/ip_forward`的值为1
2. `iptables -t nat -A POSTROUTING -o eth0 -j MASQUERADE` 设置eth0为出口
3. `iptables -t nat -A POSTROUTING -o eth1 -j MASQUERADE` 设置eth1为出口
4. `iptables -t nat -A PREROUTING -i eth0 -p tcp --dport 80 -j DNAT --to-destination 192.168.1.1:80` 设置eth0的80端口转发到192.168.1.1的80端口
5. `iptables -t nat -A PREROUTING -i eth1 -p tcp --dport 80 -j DNAT --to-destination 192.168.1.1:80` 设置eth1的80端口转发到192.168.1.1的80端口
6. 如果还有一个eth2,在eth1与eth2之间，需要优先转发到eth2, 则使用命令

此时发现电脑仍不通，需要设置电脑的网关。

### yocto下载包后编译仍会重新fetch的问题

1. 执行build时下载包会被先清理掉了，导致重新下载
1. 要执行`bitbake -c fetch package`, 完成xxx.done文件创建

## 12/9

1. modemserver运行在后台时popen(cmd)指令执行无输出, 抓取SIGCHLD信号可见。
2. modemserver需要等待grpc启动时间，测量方法
3. 设置UART打印不suspend

    ```sh
    root@fvt-5g-tbox:/# ls /sys/module/printk/parameters/ -l
    -rw-r--r--    1 root     root          4096 Jan  1 00:20 always_kmsg_dump
    -rw-r--r--    1 root     root          4096 Jan  1 00:20 console_no_auto_verbose
    -rw-r--r--    1 root     root          4096 Jan  1 00:22 console_suspend
    -rw-r--r--    1 root     root          4096 Jan  1 00:20 ignore_loglevel
    -rw-r--r--    1 root     root          4096 Jan  1 00:20 time
    ```

4. 解决log后续的parse来不及时已经由log service exit()了的问题

## 12/10

1. systemctl hostapd-ap0服务启动异常
   1. /etc/hostapd_ap0.conf配置异常
   2. ap0开机后有可能拉不起来，导致hostapd程序异常退出，原因未知

1. 撰写《SOA》

## 12/11

1. modem setslot打印太多，屏蔽

1. hostapd-ap0服务启动异常原因
   1. wifi驱动是以module的形式加载，其加载是在uevent事件触发wlan设备发现事件之后，时间比较靠后，故hostapd服务启动太快而ko仍未加载成功导致ap0拉不起来

1. 编写`fii-coredump.sh`

    ```sh
    #!/bin/bash

    core_down()
    {
        sed -i '/ulimit -c 2048/culimit -c 0' /root/.bashrc
        echo "Set off coredump"
        echo "logout and login again to make it work"
    }

    core_up()
    {
        grep 'ulimit -c' /root/.bashrc >> /dev/null
        if [ $? -eq 0 ];then
            sed -i '/ulimit -c 0/culimit -c 2048' /root/.bashrc
        else
            echo "ulimit -c 2048" >> /root/.bashrc
        fi
        echo "/home/logs/core-%e-%p-%t" > /proc/sys/kernel/core_pattern
        echo "Set on coredump, the coredump log will place in /home/logs"
        echo "logout and login again to make it work"
    }

    case "$1" in
        "up")
                core_up
                ;;
        "down")
                core_down
                ;;
        *)
                echo $0 up/down
                ;;
    esac
   ```

1. 解决客户端无法获取dns的问题
   1. TI板子服务端使用的是udhcpd做dhcp和dns服务，但配置文件未设置分配dns功能如下：

   ```sh
   [docker] f1339899@lhbvm176:working_dir$ cat fii-rootfs/etc/udhcpd_ap0.conf   
    start 192.168.1.2
    end 192.168.1.254
    interface ap0
    max_leases 250
    opt router 192.168.1.1
    option subnet 255.255.255.0
   ```

   1. 在文件中加入如下字段，则分配8.8.8.8 or 114.114.114.114给客户端：

   ```sh
   option dns 8.8.8.8,114.114.114.114
   ```

1. 协助解决GNSS tcp包问题

## 12/12

1. 修改cn.fii --> org.fii

1. emac rx handle 计数

## 12/13

1. gpio引脚在deep sleep后的悬浮问题，确认SOC侧正常，由电路设计导致

1. 指导新人编程

1. 升级hostapd和wpa_supplicant。

## shcedule

TODO: 休眠唤醒策略

