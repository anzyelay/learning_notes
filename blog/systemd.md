
# 参考文章

## 1. [Systemd 入门教程：命令篇](https://www.ruanyifeng.com/blog/2016/03/systemd-tutorial-commands.html)

## 2. [Systemd 入门教程：实战篇](https://www.ruanyifeng.com/blog/2016/03/systemd-tutorial-part-two.html)

![图片.png](https://cdn.nlark.com/yuque/0/2021/png/22279344/1631265055969-cd3978e5-bfbc-4e7d-b463-3443b6e27e98.png#clientId=ue2af73b3-1d52-4&crop=0&crop=0&crop=1&crop=1&from=paste&height=405&id=u695b478e&margin=%5Bobject%20Object%5D&name=%E5%9B%BE%E7%89%87.png&originHeight=405&originWidth=720&originalType=binary&ratio=1&rotation=0&showTitle=false&size=63786&status=done&style=none&taskId=u691cb8be-2e3b-4a26-afab-284e83fd0dd&title=&width=720)

## 3. [容易懂的 Systemd 服务管理教程](https://cloud.tencent.com/developer/article/1516125)

# systemd 知识点

## system manager instance vs user manager instances
systemd的启动的实例分两类:system manager instance是在boot下启动的第一个进程，这之后systemd manager instance会根据需要启动user manager instances（`man user@.service`）。**查看配置修改user的服务必须在命令中加个--user的option选项才行**。
```bash
$ systemctl --user enable myuser.service 
$ systemctl --user daemon-reload 
$ systemctl --user start myuser.service
```

##  查找配置目录

- system unit directories:`pkg-config systemd --variable=systemdsystemunitdir`
- system configuration directory:`pkg-config systemd --variable=systemdsystemconfdir`
- user unit directories:`pkg-config systemd --variable=systemduserunitdir`
- user configuration directory:`pkg-config system --variable=systemduserconfdir`

详见：`man systemd.unit`<br />
![图片.png](https://cdn.nlark.com/yuque/0/2021/png/22279344/1631523080311-48dfa3ac-7e18-4d86-8e52-caa96e27b935.png#clientId=ubde14ad4-5364-4&crop=0&crop=0&crop=1&crop=1&from=paste&height=817&id=ue73c9361&margin=%5Bobject%20Object%5D&name=%E5%9B%BE%E7%89%87.png&originHeight=817&originWidth=1342&originalType=binary&ratio=1&rotation=0&showTitle=false&size=195483&status=done&style=none&taskId=u34dc5218-d874-47e0-a1ec-7daaf37787c&title=&width=1342)
![图片.png](https://cdn.nlark.com/yuque/0/2021/png/22279344/1631523086927-1df940d4-125b-4ac9-aebb-b63c5e976c7f.png#clientId=ubde14ad4-5364-4&crop=0&crop=0&crop=1&crop=1&from=paste&id=u5981ca69&margin=%5Bobject%20Object%5D&name=%E5%9B%BE%E7%89%87.png&originHeight=817&originWidth=1342&originalType=binary&ratio=1&rotation=0&showTitle=false&size=195483&status=done&style=none&taskId=u81d523ca-599e-4cf4-a597-d6c323fb502&title=)

## 常用命令

- **systemd-cgls** -- 树形递归显示控制组内容,可用于查看当前用户下所有进程所在的服务单元unit
```bash
ann@B460M-d5c6e3b7:switchboard$ systemd-cgls | grep audio
│   │ ├─pulseaudio.service 
│   │ │ └─5769 /usr/bin/pulseaudio --daemonize=no --log-target=journal
│     └─24283 grep --color=auto audio

```

- systemctl enable/disable: 使能某项unit,本质就是在相应启动配置中ln创建删除软链接
- systemctl list-dependencies [--reverse ] unit :寻找unit的依赖服务 [寻找依赖此unit的服务 ]
```bash
ann@B460M-d5c6e3b7:switchboard$ systemctl --user list-dependencies pulseaudio.service --all --reverse 
pulseaudio.service
● └─default.target

ann@B460M-d5c6e3b7:switchboard$ systemctl --user list-dependencies default.target 
default.target
● ├─pulseaudio.service
● └─basic.target
●   ├─paths.target
●   ├─sockets.target
●   │ ├─dbus.socket
●   │ ├─dirmngr.socket
●   │ ├─gpg-agent-browser.socket
●   │ ├─gpg-agent-extra.socket
●   │ ├─gpg-agent-ssh.socket
●   │ ├─gpg-agent.socket
●   │ ├─pk-debconf-helper.socket
●   │ └─pulseaudio.socket
●   └─timers.target

```







# 开关机流程

## 1. SYSTEM MANAGER BOOTUP![图片.png](https://cdn.nlark.com/yuque/0/2021/png/22279344/1631520259926-4639c7cb-236b-487c-9365-cd1896d2047e.png#clientId=ubde14ad4-5364-4&crop=0&crop=0&crop=1&crop=1&from=paste&height=503&id=ue31bd4bc&margin=%5Bobject%20Object%5D&name=%E5%9B%BE%E7%89%87.png&originHeight=503&originWidth=811&originalType=binary&ratio=1&rotation=0&showTitle=false&size=44398&status=done&style=none&taskId=u4a8aaa5b-7760-49d2-b87a-93c22ab4859&title=&width=811)

## 2. USER MANAGER STARTUP
![图片.png](https://cdn.nlark.com/yuque/0/2021/png/22279344/1631520309742-0d3eb6dd-839b-43d0-9a0c-be975f02120d.png#clientId=ubde14ad4-5364-4&crop=0&crop=0&crop=1&crop=1&from=paste&height=482&id=ubc5e7769&margin=%5Bobject%20Object%5D&name=%E5%9B%BE%E7%89%87.png&originHeight=482&originWidth=886&originalType=binary&ratio=1&rotation=0&showTitle=false&size=69466&status=done&style=none&taskId=u8eba1016-7190-45b5-a2d6-a60279f8849&title=&width=886)

## 3. SYSTEM MANAGER SHUTDOWN
![图片.png](https://cdn.nlark.com/yuque/0/2021/png/22279344/1631519105375-3d70eda1-7e5e-43cb-b90f-2cfeeb5e91b6.png#clientId=ubde14ad4-5364-4&crop=0&crop=0&crop=1&crop=1&from=paste&height=633&id=YNeq9&margin=%5Bobject%20Object%5D&name=%E5%9B%BE%E7%89%87.png&originHeight=633&originWidth=1257&originalType=binary&ratio=1&rotation=0&showTitle=false&size=83032&status=done&style=none&taskId=u24648f0c-8cb9-4877-a4f5-6ffabe61496&title=&width=1257)<br />参见`man systemd.special`的shutdown.target
>      shutdown.target
>            A special target unit that terminates the services on system shutdown.
> 
>            Services that shall be terminated on system shutdown shall add Conflicts= and Before= dependencies to this unit for their service unit, which is implicitly done when DefaultDependencies=yes is set (the default).
> 

shutdown conflicts with all system services：指那些定义了Conflicts=shutdown.target 和 Before=shutdown.target 依赖关系的服务在shutdown.target运行之前会停止，（默认情况下服务单元设置DefaultDependencies=yes ，将会自动添加shutdown.target依赖而不必手动添加conflict,before）。 <br />[<br />](https://blog.csdn.net/z1026544682/article/details/104538239)

## 参考
`man bootup`,或查看[在线手册](https://www.linux.org/docs/man7/bootup.html)
