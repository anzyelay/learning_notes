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

1. 要执行`bitbake -c package`
