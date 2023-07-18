- [名词解释](#名词解释)
- [Docker 镜像使用](#docker-镜像使用)
- [容器使用](#容器使用)
- [docker仓库管理](#docker仓库管理)
- [代理设置](#代理设置)
- [示例](#示例)

------

## 名词解释

- 镜像： Docker 镜像（Image），就相当于是一个 root 文件系统
- 容器： 容器是镜像运行时的实体
- 仓库（Repository）：仓库可看成一个代码控制中心，用来保存镜像。

## docker 配置

1. Add user to group

    ```sh
    sudo usermod -aG docker $USER
    ```

## Docker 镜像使用

1. 查找镜像: `docker search imagename`

1. 获取一个新的镜像： `docker pull imagename:tag[@sha_number]`

1. 删除镜像: `docker rmi IMAGE`

1. 列出镜像: `docker images`

   ```sh
   runoob@runoob:~$ docker images           
    REPOSITORY          TAG                 IMAGE ID            CREATED             SIZE
    ubuntu              14.04               90d5884b1ee0        5 days ago          188 MB
    php                 5.6                 f40e9e0f10c8        9 days ago          444.8 MB
    nginx               latest              6f8d099c3adc        12 days ago         182.7 MB
   ```

   各个选项说明:
    - REPOSITORY：表示镜像的仓库源
    - TAG：镜像的标签
    - IMAGE ID：镜像ID, 可使用此id运行，删除，提交
    - CREATED：镜像创建时间
    - SIZE：镜像大小

    REPOSITORY:TAG 来定义不同的镜像，一个仓库可以有多个TAG

1. 创建镜像:

   - 从已经创建的容器中更新镜像，并且提交这个镜像: `docker commit -m="commit description" -a="author" 容器id  镜像名`
   - 从零开始创建新镜像，先为此创建一个Dockerfile文件，包含一组指令来告知docker如何创建一个新的镜像: `docker build -t 镜像名 dockerfile_dir`

    镜像名形如：anzye/ubuntu:v2;  **dockerfile_dir**是指dockerfile所在的目录，也可以是绝对路径

    [dockerfile格式](https://www.runoob.com/docker/docker-dockerfile.html)：

    ```sh
    runoob@runoob:~$ cat Dockerfile 
    FROM    centos:6.7
    MAINTAINER      Fisher "fisher@sudops.com"

    RUN     /bin/echo 'root:123456' |chpasswd

    # customize
    # adduser leixa 
    RUN useradd leixa -u 1001 -d /home/leixa -m -s /bin/bash
    # change password
    RUN echo "leixa:lxa123" | chpasswd


    RUN     /bin/echo -e "LANG=\"en_US.UTF-8\"" >/etc/default/local
    EXPOSE  22
    EXPOSE  80
    WORKDIR /home/

    CMD     /usr/sbin/sshd -D
    ```

      1. 每一个指令都会在镜像上创建一个新的层，每一个指令的前缀都必须是大写的。
      2. 第一条FROM，指定使用哪个镜像源
      3. RUN 指令告诉docker 在镜像内执行命令，安装了什么。。。
      4. WORKDIR: 指示登录后的目录所在
      5. CMD:容器启动后运行的指令

1. 设置镜像标签: `docker tag SOURCE_IMAGE[:TAG] TARGET_IMAGE[:TAG]`

    eg: 镜像id为860c279d2fec多了一个dev的tag

    ```sh
    runoob@runoob:~$ docker tag 860c279d2fec runoob/centos:dev
    runoob@runoob:~$ docker images
    REPOSITORY          TAG                 IMAGE ID            CREATED             SIZE
    runoob/centos       6.7                 860c279d2fec        5 hours ago         190.6 MB
    runoob/centos       dev                 860c279d2fec        5 hours ago         190.6 MB
    runoob/ubuntu       v2                  70bf1840fd7c        22 hours ago        158.5 MB
    ubuntu              14.04               90d5884b1ee0        6 days ago          188 MB
    ```

## 容器使用

1. 查看已运行容器: `docker ps [-al]`

1. 停止一个容器: `docker stop container_id`

1. 删除容器: `docker rm container_id`

1. 启动容器

    1. 启动新容器：`docker run [OPTIONS] IMAGE [COMMAND] [ARG...]`

        option说明：
        - i: 交互式操作
        - t: 终端
        - v: 绑定挂载卷, "-v localhost_path:container_path[:ro]", 只读卷在末尾加上":ro",默认为rw
        - --name: 容器名称
        - p: 指定端口映射,"-p localhost_port:container_port"
        - d: 后台运行
        - '--privileged': 解决在容器内的权限问题
        - e: 指定环境变量: 可参考[mydocker.sh](../script/mydocker.sh)

    1. 启动已存在容器：`docker start container_id`

    1. 重启容器:`docker restart container_id`

1. 进入容器  
    在使用 -d 参数时，容器启动后会进入后台。此时想要进入容器，可以通过以下指令进入:
    - `docker attach container_id`: 退出容器时会停止容器
    - `docker exec -it container_id /bin/bash`：从这个容器退出，容器不会停止

1. 导出和导入容器

    ```sh
    #导出容器
    $ docker export 1e560fca3906 > ubuntu.tar
    #导入容器way-1
    $ cat docker/ubuntu.tar | docker import - test/ubuntu:v1
    #导入容器way-2
    $ docker import http://example.com/ubuntu.tgz example/imagerepo
    #导入容器way-3
    $ docker import /tmp/ubuntu.tgz example/imagerepo
    ```

1. 为了容器或宿主机重启后自动启动，需要设置如下

    宿主机： docker update –restart=always 容器ID(或者容器名)

## docker仓库管理

1. 镜像加速器：https://www.runoob.com/docker/docker-mirror-acceleration.html
2. 登录:`docker login -u name`
3. 退出:`docker logout`
4. 推送镜像：`docker push username/ubuntu:18.04`

    ```sh
    $ docker push username/ubuntu:18.04
    $ docker search username/ubuntu

    NAME             DESCRIPTION       STARS         OFFICIAL    AUTOMATED
    username/ubuntu
    ```

## 代理设置

- 拉取镜像时的代理设置： [Configure the daemon with systemd](https://docs.docker.com/config/daemon/systemd/)

- 创建的容器内的代理设置： [Configure Docker to use a proxy server](https://docs.docker.com/network/proxy/)

## 示例

1. Dockerfile文件如下:

   ```Dockerfile
   ############################################################
    # Dockerfile to build KunLun complie container images
    # Based on Ubuntu 16.04
    ############################################################

    # Set the base image to Ubuntu
    FROM ubuntu:21.04

    # File Author / Maintainer
    MAINTAINER Lei xiaoan, leixa@jideos.com

    ################## BEGIN INSTALLATION ######################
    RUN sed -i 's/archive.ubuntu.com/repo.huaweicloud.com/g' /etc/apt/sources.list
    RUN apt update && apt install -y sudo \
        openssh-server \
        vim bash-completion \
        man bc \
        repo git ssh make gcc libssl-dev liblz4-tool  \
        expect g++ patchelf chrpath gawk texinfo  \
        chrpath diffstat binfmt-support  \
        qemu-user-static live-build bison flex \
        fakeroot cmake gcc-multilib g++-multilib \
        unzip device-tree-compiler ncurses-dev time
    # dont use mutilpline RUN apt install -y xxx
    # because of that every RUN command is a new layer of container

    RUN mkdir -p /var/run/sshd

    RUN useradd jide -u 1000 -d /home/jide -m -s /bin/bash
    RUN echo "jide:123" | chpasswd
    #############################################################

    # Expose ports
    EXPOSE 22

    # customize
    # adduser leixa 
    RUN useradd leixa -u 1001 -d /home/leixa -m -s /bin/bash

    # change password
    RUN echo "leixa:lxa123" | chpasswd

    # create directory
    RUN mkdir -p /home/jide/rk3588_sdk
    RUN chown -R jide:jide /home/jide/rk3588_sdk
    WORKDIR /home/jide/rk3588_sdk

    CMD     /usr/sbin/sshd -D
   ```

2. 使用Dockerfile生成 "**ubuntu:rk3588**" 镜像：

    `docker build -t ubuntu:rk3588 /your/dockerfile/path（不是文件名，只是路径名，会自动搜索Dockerfile）`

    镜像基于ubuntu 21.04构建，包含一个缺省用户root，密码123123，编译rk3588需要的基本环境
    建议修改Dockerfile中customize部分，添加一个与20上自己用户同名的用户和uid，否则下面共享文件会出现权限问题，可以创建自己需要的用于共享的目录

3. 以镜像**ubuntu:rk3588**创建容器**rk3588_sdk_env**：

   - 无共享文件夹:

     `docker run --name rk3588_sdk_env --privileged -v /dev:/dev -v /proc:/proc -v /dev/pts:/dev/pts -d -p 10088:22 ubuntu:rk3588 /usr/sbin/sshd -D`

   - 有共享文件夹：

     `docker run --name rk3588_sdk_env --privileged -v /dev:/dev -v /proc:/proc -v /dev/pts:/dev/pts -d -p 10088:22 -v /home/jide/rk3588:/home/jide/rk3588_sdk ubuntu:rk3588 /usr/sbin/sshd -D`

4. ssh进入容器**rk3588_sdk_env**

    `ssh -p 10088 yourname@localhost`

5. 容器有变更可先以当前容器生成新镜像，再从新创建容器

6. 如何判断是否在容器中,并更改PS标记？

    ```sh
        ps --no-headers --pid 1 | grep  --silent docker-init && in_docker=1 || in_docker=0
        [ $in_docker = 1 ] && {
            PS1=`\[\033[01;36m\][docker] `$PS1
        }
    ```

7. script exsample

   ```sh

   ```
