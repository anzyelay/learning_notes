# Install ubuntu in WSL2 with a proxy running in win11

由于window上必须用代理才能上网，故有此一文

## 设置powershell中的代理环境，使能网络以便wsl可以联网安装

用管理员权限打开powershell, 设置是无需用户名密码

1. 方法1

    ```sh
    netsh.exe winhttp import proxy source=ie
    ```

1. 方法2

    ```sh
    netsh.exe winhttp set proxy proxy-server="10.191.131.105:port" bypass-list="10.*;172.*;*.foxconn.com;*.foxconn.net;*.efoxconn.com;*.local;192.168.*.*;<local>"
    ```

*注: 不能用PING来测试网络, 因为PING使用的是ICMP协议，代理只能转发TCP和UDP。*

## 设置wsl的配置以便VM内的数据可被代理转发

1. 打开powershell终端, 进到%UserProfile%目录

    ```sh
    PS C:\Users\anzye> cd ~
    ```

1. 创建.wslconfi文件, 设置**localhostForwarding**为true, 内容如下

   ```txt
    PS C:\Users\anzye> cat .\.wslconfig
    # Settings apply across all Linux distros running on WSL 2
    [wsl2]
    # Turn off default connection to bind WSL 2 localhost to Windows localhost
    localhostforwarding=true
   ```

## 用wsl安装linux

参考 [使用 WSL 在 Windows 上安装 Linux]

```sh
wsl --install -d Ubuntu
```

## 迁移VM到其它盘

1. 终止正在运行的wsl `wsl --shutdown`

1. 查看已安装Linux distro

    ```sh
    PS C:\Users\anzye> wsl -l -v
    NAME      STATE           VERSION
    * Ubuntu    Running         2
    ```

1. 导出需要迁移的linux Distro, `wsl --export <Distro> <FileName>`

    ```sh
    wsl --export Ubuntu d:\wsl-Ubuntu.tar
    ```

1. 注销导出的Distro

   ```sh
   wsl --unregister Ubuntu
   ```

1. 导入上一步中的tar, `wsl --import <Distro> <InstallLocation> <FileName>`

    ```sh
    wsl --import Ubuntu d:\wsl-ubuntu d:\wsl-Ubuntu.tar
    ```

1. 删除tar

## 设置VM(以Ubuntu为例)中的代理

1. 在~/.bashrc中设置环境变量如下

    ```sh
    set_proxy(){
        proxy_ip=$1
        export http_proxy=http://username:passwd@$proxy_ip:port
        export HTTP_PROXY=http://username:passwd@$proxy_ip:port
        export https_proxy=http://username:passwd@$proxy_ip:port
    }
    ```

1. 设置APT代理, 如下所示内容

   ```sh
    anzye@F9899-PC-1:~/curl$ cat /etc/apt/apt.conf.d/80proxy.conf
    Acquire::http::proxy "http://username:passwd@proxy_ip:port";
   ```

## 设置开发环境

1. In Ubuntu/WSL:

    ```sh
    sudo apt install build-essential flex bison libssl-dev libelf-dev git dwarves
    git clone https://github.com/microsoft/WSL2-Linux-Kernel.git
    cd WSL2-Linux-Kernel
    cp Microsoft/config-wsl .config
    make -j $(expr $(nproc) - 1)
    ```

1. From Windows, copy `\\wsl$\<DISTRO>\home\<USER>\WSL2-Linux-Kernel\arch\x86\boot\bzimage` to your Windows profile (%userprofile%, like `C:\Users\<Windows_user>`)

1. Create the file `%userprofile%\.wslconfig` that contains:

    ```sh
    [wsl2]
    kernel=C:\\Users\\WIN10_USER\\bzimage
    ```

    *Note: The double backslashes (\\) are required. Also, to avoid a potential old bug, make sure not to leave any trailing whitespace on either line.*

1. In PowerShell, run `wsl --shutdown`, reopen wsl

## links and references

1. [WSL 中的高级设置配置](https://learn.microsoft.com/zh-cn/windows/wsl/wsl-config)

1. [使用 WSL 在 Windows 上安装 Linux](https://learn.microsoft.com/zh-cn/windows/wsl/install)
