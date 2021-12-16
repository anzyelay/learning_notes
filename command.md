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

5. lsof:查看进程打开的文件
6. fuser:查看打开文件的进程
7. ssh 
    - 远客机首先安装有openssh-server
    - 可以运行界面的ssh连接 `DISPLAY=:0 ssh -X username@ip`

8. pkg-config:查找编译前置条件或依赖等
    ```
     gcc -o test test.c `pkg-config gtk+-3.0 --cflags --libs`
    ```
9. 命令行行为
   - 上一条命令为:`!!`
   - 往上第N条命令:`!-n`
   - 上条命令的最后一个参数:`!$`
   - 忽略重定向的最后一个参数:`$_   `

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
    ```
