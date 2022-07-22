本文将向展示如何在您的Ubuntu个人电脑或Ubuntu服务器中，直接通过Ubuntu官方软件仓库来配置本地软件仓库。在您的电脑中创建一个本地软件仓库有着许多的好处。假如您有许多电脑需要安装软件 、安全升级和修复补丁，那么配置一个本地软件仓库是一个做这些事情的高效方法。因为，所有需要安装的软件包都可以通过快速的局域网连接从您的本地服务器中下载，这样可以节省您的网络带宽，降低互联网接入的年度开支 ...

您可以使用多种工具在您的本地个人电脑或服务器中配置一个Ubuntu的本地软件仓库，在本教程中，我们将为您介绍APT-Mirror。这里，我们将把默认的镜像包镜像到本地的服务器或个人电脑中，在您的本地或外置硬盘中，我们至少需要**120GB**或更多的可用空间才行。我们可以通过配置一个**HTTP**或**FTP**服务器来与本地系统客户端共享这个软件仓库。

我们需要安装Nginx 或 Apache网络服务器和APT-Mirror来使得我们的工作得以开始。下面是配置一个可工作的本地软件仓库的步骤。


# 1. 安装所需的软件包
我们需要从Ubuntu的公共软件包仓库中取得所有的软件包，然后在我们本地的Ubuntu服务器硬盘中保存它们。

首先我们安装一个Web服务器来承载我们的本地软件仓库。这里我们将安装Apache Web服务器，但你可以安装任何您中意的Web服务器。对于http协议，Web服务器是必须的。假如您需要配置ftp协议及rsync 协议，您还可以再分别额外安装FTP服务器，如proftpd, vsftpd等等和Rsync 。

```
$ sudo apt install apt-mirror
```

# 2. 配置apt-mirror
apt-mirror的配置文件位置为"/etc/apt/mirror.list",根据注释修改相应内容，一般只需要修改 base_path和更改、添加软件源, 也可以拷到其它位置，镜像时指定配置文件。  
现在，在您的硬盘上创建一个目录来保存所有的软件包。例如，我们创建一个名为ubuntu.mirror的目录，我们将在这个目录中保存所有的软件包：
```sh
$ mkdir -p ~/ubuntu.mirror				// 在用户的根目录下创建
$ cd ubuntu.mirror
$ cp /etc/apt/mirror.list ./
```
现在，打开文件~/ubuntu.mirror/mirror.list，并按照您的需求进行修改：
```sh
############# config ##################
#
set mirror_path    ~/ubuntu.mirror
# 镜像默认存放位置的根目录，具体镜像名由镜像地址确定，比如下面的镜像将保存到mirror_path/archive.ubuntu.com
# set mirror_path  $base_path/mirror
# 配置临时下载索引位置
# set skel_path    $base_path/skel
# 配置日子,URLs和MD5校验信息存储位置
# set var_path     $base_path/var

# set cleanscript $var_path/clean.sh
# 设置默认架构， 可填： amd64 或 i386，默认是和本机一个架构
# set defaultarch  <running host architecture>
# set postmirror_script $var_path/postmirror.sh
# set run_postmirror 0
# 设置下载线程数
set nthreads     20
# 是否替换URL中的波浪线，替换成%7E（HTML代码），否则会跳过不进行下载
set _tilde 0
#
############# end config ##############
 
# 镜像amd64软件仓库
deb-amd64 http://archive.ubuntu.com/ubuntu focal main restricted universe multiverse
deb-amd64 http://archive.ubuntu.com/ubuntu focal-security main restricted universe multiverse
deb-amd64 http://archive.ubuntu.com/ubuntu focal-updates main restricted universe multiverse

# 镜像arm64软件仓库
deb-arm64 http://archive.ubuntu.com/ubuntu-ports focal main restricted universe multiverse
deb-arm64 http://archive.ubuntu.com/ubuntu-ports focal-security main restricted universe multiverse
deb-arm64 http://archive.ubuntu.com/ubuntu-ports focal-updates main restricted universe multiverse

# 镜像源码
deb-src http://archive.ubuntu.com/ubuntu trusty main restricted universe multiverse
deb-src http://archive.ubuntu.com/ubuntu trusty-security main restricted universe multiverse
deb-src http://archive.ubuntu.com/ubuntu trusty-updates main restricted universe multiverse
 
# clean http://archive.ubuntu.com/ubuntu
```
**注: 您可以将上面的官方镜像服务器网址更改为离你最近的服务器的网址，可以通过访问**[Ubuntu Mirror Server](https://launchpad.net/ubuntu/+archivemirrors)**来找到这些服务器地址。假如您并不太在意镜像完成的时间，您可以沿用默认的官方镜像服务器网址。**

这里，我们将要镜像最新和最大的 Ubuntu LTS 发行版 --- 即 Ubuntu 20.04 LTS (focal) --- 的软件包仓库，所以在上面的配置中发行版本号为focal。假如我们需要镜像其他的Ubuntu发行版本，请修改上面的focal为相应的代号。

# 3.运行第一次同步
 现在，我们必须运行apt-mirror来下载或镜像官方仓库中的所有软件包。
```
$ apt-mirror ./mirror.list
```
# 4. 配置定时同步
我们当然不能，每天自己手动同步镜像啦。我们需要配置apt-mirror每天定时同步，其实就是配置cron，cron具体讲解请参看： Linux 定时重复执行任务 crontab 命令详解

这里apt-mirror提供了cron模板文件，在/etc/cron.d/apt-mirro 中，可以cat一下来查看这里我们直接通过命令行配置：

`# sudo crontab -e`

然后增加一行
```txt
0 4 * * * apt-mirror /usr/bin/apt-mirror path/to/mirror.list > /var/spool/apt-mirror/var/cron.log
```

# 5. 安装Nginx，配置HTTP访问
使用docker容器来部署deb仓库, 容器的安装参见[docker](../docker/docker.md)，
1. 安装带Nginx的容器:`sudo docker pull nginx:1.10`
2. 运行容器, 将仓库数据映射到容器中的/usr/share/nginx/html目录。
   ```sh
   sudo docker run \
    --name local-repository \
    --restart unless-stopped \
    -p 8888:80 \
    -v ~/ubuntu.mirror/archive.ubuntu.com:/usr/share/nginx/html \
    -d \
    nginx:1.10
   ```
3. 修改配置使其可访问目录   
    此时访问 “http://服务器IP:8888” 即可访问到你发布的镜像，此时数据文件可访问，但目录不可浏览显示403错误，需要更改ngnix配置。进入容器，找到对应配置文件"/etc/nginx/conf.d/default.conf",在其中加入“autoindex on;”即可，如下

    ```sh
    server {
        listen       80;
        server_name  localhost;

        #access_log  /var/log/nginx/host.access.log  main;
            #不加是不会显示目录的
        autoindex on;
        location / {
            root   /usr/share/nginx/html;
            index  index.html index.htm;
        }

        #error_page  404              /404.html;

    }

    ```
    重启容器即可：
    ```sh
    docker restart local-repository
    ```
# 问题
1. [无法下载cnf问题](https://blog.csdn.net/gh0st007/article/details/109279872)
   > add_url_to_download( $url . $_ . "/cnf/Commands-" . $arch . ".xz" )  
   > 添加到/usr/bin/apt-mirror的第450行    
   新版的apt-mirror已解决此问题，建议从github上下载安装
   
# 参考
1. https://blog.csdn.net/anzhuangguai/article/details/62041603
