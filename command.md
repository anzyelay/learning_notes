1. 全屏播放命令
    ```sh
    ffplay -fs ~/Downloads/Big_Buck_Bunny_1080_10s_2MB.mp4  -noborder -an -sn
    ```
1.  vi 替换
    ```bash
    :%s/search/repl/flags
    ``` 
    flags:
    - &: Keep the flags from the previous substitute
            command
    - g: global
    - c: Confirm each substitution
      - y:replace
      - n:skip
    - i: ignore case
1. find:
   ```sh
   # {}代表找到的文件结果名, 安装多级目录文件
   find build/ -type f -exec install -Dm 755 "{}" "/tmp/{}" \;
   ```

2. lsof:查看进程打开的文件
3. fuser:查看打开文件的进程
4. ssh 
    - 远客机首先安装有openssh-server
    - 可以运行界面的ssh连接 `DISPLAY=:0 ssh -X username@ip`

5. pkg-config:查找编译前置条件或依赖等
    ```
     gcc -o test test.c `pkg-config gtk+-3.0 --cflags --libs`
    ```
6. 命令行行为
   - 上一条命令为:`!!`
   - 往上第N条命令:`!-n`
   - 上条命令的最后一个参数:`!$`
   - 忽略重定向的最后一个参数:`$_   `
1. read读文件，每次读取一行，直到读取不到返回非0：
```sh
cat tmp.txt  | while read line; do echo $line; done

while read name; do
    # Do what you want to $name
done < filename
```

## gtk开发命令
1. gtk-update-icon-cache
2. glib-compile-schemas
3. glib-compile-resources
4. update-fonts-dir,update-fonts-scale,mkfontscale,mkfontdir
5. 

## linux 三剑客相关
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
