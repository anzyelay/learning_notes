# Build Your Own Docker with Linux Namespaces, cgroups, and chroot: Hands-on Guide | Akash Rajpurohit

## Introduction - 开篇介绍

>Containerization has transformed the world of software development and deployment. [Docker], a leading containerization platform, leverages Linux [namespaces], [cgroups], and [chroot] to provide robust isolation, resource management, and security.

容器化改变了软件开发和部署的世界。 [Docker]是领先的容器化平台，利用 Linux 命名空间、cgroup 和 chroot 来提供强大的隔离、资源管理和安全性。

>In this guide, we’ll skip the theory (go through the attached links above if you want to learn more about the mentioned topics) and jump straight into the practical implementation.

在本指南中，我们将跳过理论（如果您想了解有关上述主题的更多信息，请浏览上面的附加链接）并直接跳到实际实施。

注:

>> Before we dive into building our own Docker-like environment using namespaces, cgroups, and chroot, it’s important to clarify that this hands-on guide is not intended to replace docker and its functionality.
>>
>> Docker have features such as layered images, networking, container orchestration, and extensive tooling that make it a powerful and versatile solution for deploying applications.
>>
>> The purpose of this guide is to offer an educational exploration of the foundational technologies that form the core of Docker. By building a basic container environment from scratch, we aim to gain a deeper understanding of how these underlying technologies work together to enable containerization.
>>
> 在我们深入使用命名空间、cgroup 和 chroot 构建我们自己的类似 Docker 的环境之前，需要澄清的是，本实践指南无意取代 docker 及其功能。
>
>Docker 具有分层镜像、网络、容器编排和广泛的工具等功能，使其成为部署应用程序的强大且多功能的解决方案。
>
>本指南的目的是对构成 Docker 核心的基础技术进行教育性探索。 通过从头开始构建基础容器环境，我们的目标是更深入地了解这些底层技术如何协同工作以实现容器化。

## Let’s build Docker - 咱们开始一起来构建Docker吧

### Step 1: Setting Up the Namespace

> To create an isolated environment, we start by setting up a new namespace.

为了创建一个隔离的环境，我们首先设置一个新的namespace

> We use the `unshare` command, specifying different namespaces `(--uts, --pid, --net, --mount, and --ipc)`, which provide separate instances of system identifiers and resources for our container.

我们使用命令`unshare`，指定一个不同的namespaces`(--uts, --pid, --net, --mount, and --ipc)`, 它为我们的容器提供了独立的系统标识和资源实例

```sh
unshare --uts --pid --net --mount --ipc --fork
```

> Read more in depth about [unshare command on man page ↗️](https://man7.org/linux/man-pages/man1/unshare.1.html "unshare - run program in new namespaces")

需要更深入的了解请参阅[unshare command on man page](https://man7.org/linux/man-pages/man1/unshare.1.html "unshare - run program in new namespaces")

### Step 2: Configuring the cgroups

> Cgroups (control groups) help manage resource allocation and control the usage of system resources by our containerized processes.
>
> We will create a new cgroup for our container and assign CPU quota limits to restrict its resource usage.

Cgroups帮助管理资源申请并控制我们容器化的进程对系统资源的使用

现在来为我们的容器创建一个新的控制组并分配受限的CPU配额来限制其资源的使用

```sh
mkdir /sys/fs/cgroup/cpu/container1
echo 100000 > /sys/fs/cgroup/cpu/container1/cpu.cfs_quota_us
echo 0 > /sys/fs/cgroup/cpu/container1/tasks
echo $$ > /sys/fs/cgroup/cpu/container1/tasks
```

>On the first line we create a new directory named container1 within the /sys/fs/cgroup/cpu/ directory. This directory will serve as the cgroup for our container.

第一行我们在`/sys/fs/cgroup/cpu/`下创建了一个名为container1的新目录。 这目录将为我们的容器提供cgroup服务

> On the second line we write the value 100000 to the cpu.cfs_quota_us file within the /sys/fs/cgroup/cpu/container1/ directory. This file is used to set the CPU quota limit for the cgroup.

第二行，我们向`/sys/fs/cgroup/cpu/container1`下的文件cpu.cfs_quota_us写入值100000。 此文件用来为组控制设置CPU配额限制

> On the third line we write the value 0 to the tasks file within the /sys/fs/cgroup/cpu/container1/ directory. The tasks file is used to control which processes are assigned to a particular cgroup.
>
> By writing 0 to this file, we are removing any previously assigned processes from the cgroup. This ensures that no processes are initially assigned to the container1 cgroup.

第三行我们向`/sys/fs/cgroup/cpu/container1`下的文件tasks写入值0。 此tasks文件用来为控制哪个进程被分配到这一特定组别来。

通过写入0到上述文件， 我们可以将前面被分配到此控制组别的任何进程删除，保证初始化后的container1组别没有任何进程

> And lastly, on the fourth line we write the value of $$ to the tasks file within the /sys/fs/cgroup/cpu/container1/ directory.
>
> $$ is a special shell variable that represents the process ID (PID) of the current shell or script. By this, we are assigning the current process (the shell or script) to the container1 cgroup.
>
> This ensures that any subsequent child processes spawned by the shell or script will also be part of the container1 cgroup, and their resource usage will be subject to the specified CPU quota limits.

最后， 第四行我们写入`$$`到`/sys/fs/cgroup/cpu/container1`下的tasks文件中， `$$`是一个代表当前运行shell或脚本进程的特殊shell变量， 通过写入此值，我们将当前的进程号分配到container1的控制组别中。
这可确保shell或脚本生成的任何后续子进程也将成为 container1 cgroup 的一部分，并且它们的资源使用将受到指定的 CPU 配额限制。

### Step 3: Building the Root File System

> To create the file system for our container, we use `debootstrap` to set up a minimal Ubuntu environment within a directory named **"ubuntu-rootfs"**.
>
> This serves as the root file system for our container.

为了给我们的容器构造一个文件系统， 我们使用`debootstrap`在名为**ubuntu-rootfs**的目录下设置一个最轻的ubuntu环境

```sh
debootstrap focal ./ubuntu-rootfs http://archive.ubuntu.com/ubuntu/
```

> The first argument `focal` specifies the Ubuntu release to install. In this case, we are installing Ubuntu 20.04 (Focal Fossa) ↗️.
>
> The second argument `./ubuntu-rootfs` specifies the directory to install the Ubuntu environment into. In this case, we are installing it into the ubuntu-rootfs directory.
>
> The third argument `http://archive.ubuntu.com/ubuntu/` specifies the URL of the Ubuntu repository to use for the installation.
>
> More about debootstrap can be read on the man page ↗️

第一个参数`focal`指定安装的ubuntu版本， 此例我们安装ubuntu20.04
第二个参数`./ubuntu-rootfs`指定ubuntu环境的安装目录， 此例我们安装到目录ubuntu-rootfs下。
第三个参数`http://archive.ubuntu.com/ubuntu/`指定安装ubuntu的仓库来源
`debootstrap`更多功能请参见[read on the man page](https://linux.die.net/man/8/debootstrap "debootstrap - Bootstrap a basic Debian system")

### Step 4: Mounting and Chrooting into the Container

> We mount essential file systems, such as /proc, /sys, and /dev, within our container’s root file system.
>
> Then, we use the chroot command to change the root directory to our container’s file system.

我们需要在我们的容器根目录下挂载必需的文件系统，比如： /proc, /sys和/dev，然后我们用`chroot`命令更改当前的根目录到我们的容器文件系统去

```sh
mount -t proc none ./ubuntu-rootfs/proc
mount -t sysfs none ./ubuntu-rootfs/sys
mount -o bind /dev ./ubuntu-rootfs/dev
chroot ./ubuntu-rootfs /bin/bash
```

> The first command mounts the proc filesystem into the ./ubuntu-rootfs/proc directory. The proc filesystem provides information about processes and system resources in a virtual file format.
>
> Mounting the proc filesystem in the specified directory allows processes within the ./ubuntu-rootfs/ environment to access and interact with the system’s process-related information.
>
> The next command mounts the sysfs filesystem into the ./ubuntu-rootfs/sys directory. The sysfs filesystem provides information about devices, drivers, and other kernel-related information in a hierarchical format.
>
> Mounting the sysfs filesystem in the specified directory enables processes within the ./ubuntu-rootfs/ environment to access and interact with system-related information exposed through the sysfs interface.
>
> Finally we bind the /dev directory to the ./ubuntu-rootfs/dev directory. The /dev directory contains device files that represent physical and virtual devices on the system.
>
> By binding the /dev directory to the ./ubuntu-rootfs/dev directory, any device files accessed within the ./ubuntu-rootfs/ environment will be redirected to the corresponding devices on the host system.
>
> This ensures that the processes running within the ./ubuntu-rootfs/ environment can interact with the necessary devices as if they were directly accessing them on the host system.
>
> Once we have mounted the necessary file systems, we use the chroot command to change the root directory to the ./ubuntu-rootfs/ directory. Think of this as doing docker exec into the container.

第一条命令挂载`proc`文件系统到`./ubuntu-rootfs/proc`目录下，`proc`文件系统以虚拟文件格式提供进程和系统资源相关的信息

将`proc`文件系统挂载到指定目录中，允许`./ubuntu-rootfs/`环境中的进程访问系统进程相关信息并与之交互。

下一个命令将 `sysfs` 文件系统安装到 `./ubuntu-rootfs/sys` 目录中。 `sysfs` 文件系统以分层格式提供有关设备、驱动程序和其他内核相关信息的信息。

将 `sysfs` 文件系统挂载到指定目录中，使 `./ubuntu-rootfs/` 环境中的进程能够访问通过 `sysfs` 接口公开的系统相关信息并与之交互。

最后我们将 `/dev` 目录绑定到 `./ubuntu-rootfs/dev` 目录。 `/dev` 目录包含代表系统上的物理和虚拟设备的设备文件。

通过将 `/dev` 目录绑定到 `./ubuntu-rootfs/dev` 目录，在 `./ubuntu-rootfs/` 环境中访问的任何设备文件都将被重定向到主机系统上相应的设备。

这确保了在`./ubuntu-rootfs/` 环境中运行的进程可以与必要的设备进行交互，就像它们直接在主机系统上访问它们一样。

一旦我们安装了必要的文件系统，我们就使用 `chroot` 命令将根目录更改为 `./ubuntu-rootfs/` 目录。 这可以视为在容器中执行 `docker exec`。

### Step 5: Running Applications within the Container

>Now that our container environment is set up, we can install and run applications within it.
>
>In this example, we install Nginx web server to demonstrate how applications behave within the container.

到此我们的容器环境都设置好了，我们可以在里面安装和运行应用

例如，我们以安装Nginx网页服务来说明应用在容器中是如何运行的

```sh
(container) $ apt update
(container) $ apt install nginx
(container) $ service nginx start
```

## Conclusion - 结语

> In this guide, we built a basic Docker-like environment using Linux namespaces, cgroups, and chroot. We explored the code and command examples to gain a deeper understanding of how these technologies work together to create isolated and efficient containers.

在本指南中，我们使用 Linux 命名空间、cgroup 和 chroot 构建了一个基本的类似 Docker 的环境。 我们探索了代码和命令示例，以更深入地了解这些技术如何协同工作来创建隔离且高效的容器。

[Docker]: https://www.docker.com/

[namespaces]: https://akashrajpurohit.com/blog/linux-namespaces-isolating-your-system-for-enhanced-security-and-performance/

[cgroups]: https://akashrajpurohit.com/blog/linux-control-groups-finetuning-resource-allocation-for-optimal-system-performance/

[chroot]: https://akashrajpurohit.com/blog/how-to-create-a-restricted-environment-with-the-linux-chroot-command/
