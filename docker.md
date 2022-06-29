## 名词解释：
- 镜像： Docker 镜像（Image），就相当于是一个 root 文件系统
- 容器： 容器是镜像运行时的实体
- 仓库（Repository）：仓库可看成一个代码控制中心，用来保存镜像。
## Docker 镜像使用
1. 查找镜像: `docker search imagename`
1. 获取一个新的镜像： `docker pull imagename:tag[@sha_number]`
1. 删除镜像: `docker rmi IMAGE`
1. 列出镜像
 `docker images`
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
    
    dockerfile格式：
    ```sh
    runoob@runoob:~$ cat Dockerfile 
    FROM    centos:6.7
    MAINTAINER      Fisher "fisher@sudops.com"

    RUN     /bin/echo 'root:123456' |chpasswd
    RUN     useradd runoob
    RUN     /bin/echo 'runoob:123456' |chpasswd
    RUN     /bin/echo -e "LANG=\"en_US.UTF-8\"" >/etc/default/local
    EXPOSE  22
    EXPOSE  80
    CMD     /usr/sbin/sshd -D
    ```
      1. 每一个指令都会在镜像上创建一个新的层，每一个指令的前缀都必须是大写的。
      1. 第一条FROM，指定使用哪个镜像源
      1. RUN 指令告诉docker 在镜像内执行命令，安装了什么。。。
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
1. 停止一个容器: ` docker stop container_id`
1. 删除容器: `docker rm container_id`
1. 启动容器
    1. 启动新容器：`docker run [OPTIONS] IMAGE [COMMAND] [ARG...]`

        option说明：
        - i: 交互式操作
        - t: 终端
        - v: 绑定挂载卷
        - --name: 容器名称
        - p: 指定端口映射
        - d: 后台运行
    
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
