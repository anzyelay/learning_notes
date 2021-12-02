1. 全屏播放命令
    ```sh
    ffplay -fs ~/Downloads/Big_Buck_Bunny_1080_10s_2MB.mp4  -noborder -an -sn
    ```
1. 替换文件内容
    ```sh
    grep "patapus" . -rl --exclude=*{po,pot} | xargs sed -i "s/patapus/patapua/g"
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

4. grep 多词搜索
    ```sh
    grep -E "elementary|switchboard" . -rn --exclude-dir={build,.git,.github} --exclude=*.{po,pot}
    ```
5. lsof:查看进程打开的文件
6. fuser:查看打开文件的进程
7. ssh 
    - 远客机首先安装有openssh-server
    - 可以运行界面的ssh连接 `DISPLAY=:0 ssh -X username@ip`

8. pkg-config:查找编译前置条件或依赖等
    ```
     gcc -o test test.c `pkg-config gtk+-3.0 --cflags --libs`
    ```
