1. 全屏播放命令

`ffplay -fs ~/Downloads/Big_Buck_Bunny_1080_10s_2MB.mp4  -noborder -an -sn`

2. 查看当前配置状态 (build为构建目录)
`meson configure build`

3. 替换文件内容
```sh
grep "patapus" . -rl --exclude=*{po,pot} | xargs sed -i "s/patapus/patapua/g"
```

4. vi 替换
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
- 