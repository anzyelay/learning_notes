# DOCKER

- [名词解释](#名词解释)
- [Docker 镜像使用](#docker-镜像使用)
- [容器使用](#容器使用)
- [docker仓库管理](#docker仓库管理)
- [代理设置](#代理设置)
- [示例](#示例)

------

## 参考

- [Docker - 从入门到实践](https://yeasy.gitbook.io/docker_practice/basic_concept/container)
- [A sysadmin's guide to containers](https://opensource.com/article/18/8/sysadmins-guide-containers, "帮助理解容器镜像")
- [docker CLI reference](https://docs.docker.com/engine/reference/run/)

## 实现基础

1. namespace: 实现不同命名空间的资源隔离
2. cgroups： 实现系统资源的访问控制

## 名词解释

- 镜像： Docker 镜像（Image），就相当于是一个 root 文件系统
- 容器： 容器是镜像运行时的实体
- 仓库（Repository）：仓库可看成一个代码控制中心，用来保存镜像。

## 安装

```sh
curl -fsSL https://get.docker.com | bash -s docker --mirror Aliyun
```

## docker 配置

1. Add user to docker group

    ```sh
    sudo groupadd docker
    sudo usermod -aG docker $USER
    ```

    > Log out and log back in so that your group membership is re-evaluated
    > If you're running Linux in a virtual machine, it may be necessary to restart the virtual machine for changes to take effect.
    > You can also run the following command to activate the changes to groups:
    > `newgrp docker`

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
      3. **RUN** 指令告诉docker 在镜像内执行命令，安装了什么。。。
      4. **WORKDIR**: 指示登录后的目录所在
      5. **CMD**:启动容器后提供默认执行的命令,会被`docker run`命令中给出的参数所覆盖, 最后一个有效
      6. **ENTRYPOINT**: 用于定义容器启动时要执行的命令或程序, 与**CMD**不同，不会被`docker run`命令中给出的参数所覆盖, 相反，这些参数会被作为ENTRYPOINT命令的参数传递给容器, 最后一个有效

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

1. 镜像加速器：<https://www.runoob.com/docker/docker-mirror-acceleration.html>
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

## Dockerfile

用于创建docker image, 示例文件如下：

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

# 设置切换当前用户
USER leixa
# 默认启动命令
CMD     /usr/sbin/sshd -D
## 启动命令
## entrypoint.sh需和Dockerfile文件放同一目录内
#COPY entrypoint.sh /usr/bin/entrypoint.sh
#ENTRYPOINT ["/usr/bin/entrypoint.sh"]
```

1. 使用Dockerfile生成 "**ubuntu:rk3588**" 镜像：

    ```sh
    docker build -t ubuntu:rk3588 /your/dockerfile/path（不是文件名，只是路径名，会自动搜索Dockerfile）
    ```

    - 镜像基于ubuntu 21.04构建，包含一个缺省用户root，密码123，编译rk3588需要的基本环境
    - 建议修改Dockerfile中customize部分，添加一个与host主机上用户同名的用户和uid，否则下面共享文件会出现权限问题，可以创建自己需要的用于共享的目录
    - 可以将所有RUN命令写到一个脚本中执行一次RUN即可,这样达到layer层次最小化的目的

        ```Dockerfile
        ADD run.sh /tmp/run.sh
        RUN cd /tmp && chmod +x run.sh && run.sh && rm run.sh
        ```

    - 在Docker镜像中创建一个用户，我们可以在Dockerfile使用`ARG`, `RUN`和 `USER` 指令达成目的:

        ```sh
        FROM alpine:latest
        ARG DOCKER_USER=default_user
        RUN addgroup -S $DOCKER_USER && adduser -S $DOCKER_USER -G $DOCKER_USER
        USER $DOCKER_USER
        CMD ["whoami"]
        ```

        在创建时也可以方便的修改用户名:
        `$ docker build --build-arg DOCKER_USER=baeldung -t dynamicuser .`

    - path里面的所有内容在创建时都要发送给docker daemon,所以不要放太多无关的文件在path目录下，不然影响创建效率

## 示例

1. 以镜像**ubuntu:rk3588**创建容器**rk3588_sdk_env**：

   - 无共享文件夹:

     `docker run --name rk3588_sdk_env --privileged -v /dev:/dev -v /proc:/proc -v /dev/pts:/dev/pts -d -p 10088:22 ubuntu:rk3588 /usr/sbin/sshd -D`

   - 有共享文件夹：

     `docker run --name rk3588_sdk_env --privileged -v /dev:/dev -v /proc:/proc -v /dev/pts:/dev/pts -d -p 10088:22 -v /home/jide/rk3588:/home/jide/rk3588_sdk ubuntu:rk3588 /usr/sbin/sshd -D`

1. ssh进入容器**rk3588_sdk_env**

    `ssh -p 10088 yourname@localhost`

1. 容器有变更可先以当前容器生成新镜像，再从新创建容器

1. 如何判断是否在容器中,并更改PS标记？

    ```sh
    ps --no-headers --pid 1 | grep  --silent docker-init && in_docker=1 || in_docker=0
    [ $in_docker = 1 ] && {
        PS1=`\[\033[01;36m\][docker] `$PS1
    }
    ```

1. script example
    将entrypoint.sh与Dockefile放一起，创建好镜像，后用start_docker.sh创建启动容器实例

    ```sh
    #!/bin/bash
    set -e

    # User config
    msg="docker_entrypoint: Setup user config"
    echo $msg
    echo "export DEBIAN_FRONTEND=noninteractive" >> /etc/bash.bashrc
    echo $msg - "done"

    # Example:
    #  docker run -it -e USER_ID=$(id -u) -e GROUP_ID=$(id -g) -e USER_NAME=$(whoami) imagename bash

    # Reasonable defaults if no USER_ID/GROUP_ID environment variables are set.
    if [ -z ${USER_ID+x} ]; then USER_ID=9999; fi
    if [ -z ${GROUP_ID+x} ]; then GROUP_ID=9999; fi
    if [ -z ${USER_NAME+x} ]; then USER_NAME=docker; fi
    if [ -z ${GROUP_NAME+x} ]; then GROUP_NAME=docker; fi
    if [ -z ${HOME_PATH+x} ]; then HOME_PATH=/home2/$USER_NAME; fi

    msg="docker_entrypoint: Creating user UID/GID [$USER_ID/$GROUP_ID]"
    echo $msg
    groupadd -g $GROUP_ID -r $GROUP_NAME && \
    useradd -u $USER_ID -r -g $GROUP_NAME -d "$HOME_PATH" $USER_NAME
    adduser $USER_NAME sudo && usermod -aG sudo $USER_NAME && echo "$USER_NAME ALL=(ALL) NOPASSWD:ALL" >> /etc/sudoers
    echo "$msg - done"

    msg="Change account $USER_NAME shell from csh to bash"
    echo $msg
    chsh -s /bin/bash $USER_NAME
    echo "$msg - done"

    # Default to 'bash' if no arguments are provided
    args="$@"
    if [ -z "$args" ]; then
    args="bash"
    fi
    export HOME="$HOME_PATH"

    msg="Load color scheme"
    echo $msg
    if [ -e ~/.bashrc ] ; then
    test1=`grep  "alias ls" -l ~/.bashrc | wc -l`
    if [ $test1 = 0 ] ; then
        echo 'alias ls="ls --color"' >> ~/.bashrc
        echo "Load ls color successfully"
    fi

    test2=`grep  "alias ll" -l ~/.bashrc | wc -l`
    if [ $test2 = 0 ] ; then
        echo 'alias ll="ls -al --color"' >> ~/.bashrc
        echo "Load ll color successfully"
    fi

    echo "Load PS1 color successfully"
    else
    echo "Notice: ~/.bashrc is missing. please add a new one."
    fi
    msg="Load color scheme done"
    echo $msg

    msg="Welcom to AG35 build world..."
    echo $msg

    # Execute command as 'docker' user
    #cd $HOME

    #msg=`pwd`
    #echo "Enter $msg. args: $args"
    #chown -R $USER_NAME:$GROUP_NAME /opt/crosstool

    #exec sudo -u $USER_NAME $args
    exec su $USER_NAME

    $args
    ```

    start_docker.sh

    ```sh
    #!/bin/bash

    target=$1
    [ -z "$target" ] && {
            echo "Err: Need a target..."
            exit
    }

    target_image="9x07"
    image_ver="2.1"
    mount_dir=
    target="mdm9x07_kaga_$target"

    container_ret=$(docker ps -a | grep "$target")

    if [ -z "$container_ret" ]; then
            docker run --init -it --net=host \
                    -e USER_ID=$(id -u) -e USER_NAME=$(id -un) -e GROUP_ID=$(id -g) -e GROUP_NAME=$(id -gn) \
                    -e HOME_PATH=$HOME \
                    -v $HOME:$HOME -v /etc/localtime:/etc/localtime $mount_dir \
                    --name "$target" "$target_image$([ -z $image_ver ] || echo :$image_ver)" /bin/bash
    else
            echo "Container $target exist"
            ret=$(docker ps -a | grep "$target" | grep Up)
            if [ -z "$ret" ]; then
                    echo "Container $target is Off, trying to start it..."
                    docker start "$target"
            else
                    echo "Container $target is On"
            fi
            echo "Tryint to connect container $target"
            docker exec -it -u $(whoami) "$target" /bin/bash
    fi
   ```
