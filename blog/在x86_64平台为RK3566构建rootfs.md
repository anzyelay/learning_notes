# 一、环境准备
我们在x86_64的ubuntu或者类似系统上构建ISO，因此需要准备一台可跑arm64的容器的PC
## 1. 安装docker
```
$ sudo apt install -y docker.io
```

## 2. 下载debian镜像
Patapua/elementary使用Debian版本的live-build进行构建，而不是Ubuntu修改过的版本，因此可以非常容易的在Debian虚机或container里构建Patapua/elementary的iso安装镜像（live cd）。 如何获取arm镜像可参考[如何在amd平台上跑arm64的docker](./%E5%A6%82%E4%BD%95%E5%9C%A8amd%E5%B9%B3%E5%8F%B0%E4%B8%8A%E8%B7%91arm64%E7%9A%84docker.md)
```
$ docker pull --platform=arm64 debian:unstable
```

## 3. 配置debian容器
- 在debian容器中安装ssh服务器
- 修改debian容器root密码
    ```sh
    $ docker run -ti --name patapua_build debian:unstable /bin/bash
    以下命令在容器内执行
    $ ## 改源....省略
    $ apt update
    $ apt install -y git vim
    $ exit
    ```
- 提交debian容器
    ```sh
    $ docker commit patapua_build anzyelay/debian:unstable
    ```

- 运行debian容器
    ```sh
    $ sudo mount -t proc proc ./proc
    $ sudo docker run -it --privileged --name patapua_build -v ${PWD}/proc:/proc -v ${PWD}/wk:/working_dir -w /working_dir anzyelay/debian:patapua /bin/bash 
    ```
    如果不mount proc则会出现如下错误, 加了privileged也无用
    ```sh
    mount: /root/os/rk3566/patapua-arm64/proc: permission denied.
        dmesg(1) may have more information after failed mount system call.
    ```

# 二、代码获取
```sh
以下命令在容器内执行
$ git clone http://192.168.16.188:8080/patapua/os.git
$ cd os
$ git checkout test
```
# 三、rootfs构建
```
$ cd os
$ ./build.sh patapua-rk3566-rootfs
```
将在rk3566目录下生成rootfs.img文件
