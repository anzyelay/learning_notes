# command

## VI命令

1. 替换: `:%s/search/repl/flags`

    flags:
    - &: Keep the flags from the previous substitute
            command
    - g: global
    - c: Confirm each substitution
        - y:replace
        - n:skip
    - i: ignore case

1. 宏录制: `q[reg]`

    - 常规模式下按q[x]进入录制模式，[x]代表[0-9a-z]内的所有取值，指示录制的寄存器标识，下面会显示recording @x，再次按q，结束录制。 
    - 使用n@[x]进行使用,n指重复使用资料[x]是指使用哪个记录

## other

1. 全屏播放命令

    ```sh
    ffplay -fs ~/Downloads/Big_Buck_Bunny_1080_10s_2MB.mp4  -noborder -an -sn
    ```

1. find处理

   ```sh
   # {}代表找到的文件结果名, 安装多级目录文件
   find build/ -type f -exec install -Dm 755 "{}" "/tmp/{}" \;
   ```

1. lsof:查看进程打开的文件

1. fuser:查看打开文件的进程

1. ssh
    - 远客机首先安装有openssh-server
    - 可以运行界面的ssh连接 `DISPLAY=:0 ssh -X username@ip`

1. pkg-config:查找编译前置条件或依赖等

    ```sh
     gcc -o test test.c `pkg-config gtk+-3.0 --cflags --libs`
    ```

1. 命令行行为
   - 上一条命令为:`!!`
   - 往上第N条命令:`!-n`
   - 上条命令的最后一个参数:`!$`
   - 忽略重定向的最后一个参数:`$_`
   - 有管道时查看各管道的返回值:`${PIPESTATUS[n]}`,最后一个值与`$?`相同

1. read读文件，每次读取一行，直到读取不到返回非0：

```sh
cat tmp.txt  | while read line; do echo $line; done

while read name; do
    # Do what you want to $name
done < filename
```

1. `pmap`查看内存使用情况

   也可以直接查看文件`cat /proc/pid/status`或`ps -aux -q PID`

## gtk开发命令

1. gtk-update-icon-cache
1. glib-compile-schemas
1. glib-compile-resources
1. update-fonts-dir,update-fonts-scale,mkfontscale,mkfontdir

## linux 三剑客相关

| | 语法 | 特点 | option |
|--|--|--|--|
|sed | sed [option] 'pattern+action' filename | <li>找谁(pattern)干啥(action)<li>只能针对文件操作<li>针对全文匹配后干点啥 |
|[g]awk | gawk [option] [--] 'pattern {action}' | <li>找谁(pattern)干啥({action})<li>可以针对管道输出的内容<li>针对每一行匹配后干点啥，可汇总|
|grep | grep [option] patterns [filename] | <li>找谁<li>可以针对管道输出的内容|<li>-o:只显示匹配字串<li>-l:只显示匹配文件

### example

1. 替换文件内容

    ```sh
    grep "patapus" . -rl --exclude=*{po,pot} | xargs sed -i "s/patapus/patapua/g"
    ```

1. grep 多词搜索

    ```sh
    grep -E "elementary|switchboard" . -rn --exclude-dir={build,.git,.github} --exclude=*.{po,pot}
    sed -nr -e '/elementary/p' -e 'switchboard' source
    ```

1. awk '待匹配项 [!]~ /正则表达式/ { action }' ,匹配行后执行action，默认是打印

    ```sh
    dpkg --get-selections | awk '$2 ~ /^install/'
    ```

1. sed 中用"\1"代替要"\(text\)"的text值

   ```sh
   # 原文内容为： com.patapua.os-updates (1.0.0-4ubuntu1) unstable; urgency=medium
   # oldversion为 1.0.0-4ubuntu1
   OLDVERSION=$(sed -n '1s/.*(\(.*\)).*/\1/p' debian/changelog)
 
   # -r+双引号: 可以使用变量MY_DATA, 
   sed -i -r "s/^(CURRENT_TIME =).*/\1 $MY_DATE/" test.txt
   ```

1. awk 替换从管道出来的文本

   ```sh
    # 替换mac数据
    ifconfig | gawk '{ gsub(/([[:xdigit:]]{1,2}:){5}[[:xdigit:]]{1,2}/, "**:**:**:**:**:**"); print $0 }' > tmp.txt
    ifconfig | sed -r 's/([[:xdigit:]]{1,2}:){5}[[:xdigit:]]{1,2}/**:**:**:**:**:**/' /dev/stdin
    ifconfig | sed -r 's/(..):(..):(..):(..):(..):(..)/\1:**:**:\3:**:\6/' /dev/stdin
    # (..:){n}: 如果字符是“aa:bb:cc:dd:”合计有4项“xx:”形式的字符集，\1指代"dd:"，只能指代最后一项
    grep -Erl "([[:xdigit:]]{2}:){5}[[:xdigit:]]{2}" . | xargs sed -nr "s/([[:xdigit:]]{2})(:[[:xdigit:]]{2}){3}(:[[:xdigit:]]{2}){2}/\1:**:**\2:**\3/p"
   ```

## shell脚本

1. 异常退出处理

    ```sh
    finish() {
    echo "here do something before exit"
    exit -1
    }
    set -e # If not interactive, exit immediately if any untested command fails.
    trap  finish ERR
    ```

1. 脚本中输出多行提示文字

    ```sh
    cat << EOM
    hello world,
    i love u!
    EOM
    ```

1. cp进度和解压进度

   ```sh
    #Precentage function
    untar_progress ()
    {
        TARBALL=$1;
        DIRECTPATH=$2;
        BLOCKING_FACTOR=$(($(xz --robot --list ${TARBALL} | grep 'totals' | awk '{print $5}') / 51200 + 1));
        tar --blocking-factor=${BLOCKING_FACTOR} --checkpoint=1 --checkpoint-action='ttyout=Written %u%  \r' -Jxf ${TARBALL} -C ${DIRECTPATH}
    }
    untar_progress ./boot_partition.tar.xz tmp/

    #copy/paste programs
    cp_progress ()
    {
            CURRENTSIZE=0
            while [ $CURRENTSIZE -lt $TOTALSIZE ]
            do
                    TOTALSIZE=$1;
                    TOHERE=$2;
                    CURRENTSIZE=`sudo du -c $TOHERE | grep total | awk {'print $1'}`
                    echo -e -n "$CURRENTSIZE /  $TOTALSIZE copied \r"
                    sleep 1
            done
    }
    TOTALSIZE=`sudo du -c tmp/* | grep total | awk {'print $1'}`
    cp -rf tmp/* start_here/ &
    cp_progress $TOTALSIZE start_here/
    sync;sync

   ```
