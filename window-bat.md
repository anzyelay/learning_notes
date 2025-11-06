# WINDOW Bat script

## 基本

```bat
:: 关掉打印
@echo off

:: 切换到 UTF-8 代码页
chcp 65001 >nul

:: 一般是使用%value%来取变量值，但开启这个标识，可以使用!value!来取不变值
setlocal enabledelayedexpansion

: %0：表示当前批处理脚本的路径（包含文件名）
: ~d：提取驱动器盘符（drive）
: ~p：提取路径（path）
: 所以 %~dp0 表示：当前批处理文件所在的完整目录路径（带反斜杠结尾）
set "script_path=%~dp0"
echo 当前脚本路径是：%script_path%

:: 入参
set "args1=%~1"
set "args2=%~2"

:: 函数命令执行结果返回
if !errorlevel!==0 (

)

```

## 返回值

1. 直接返回值在`exit /b value`中, 通过`errorlevel`获取。
2. 间接通过入参传值出去, 需要注意的是，如果使用了`setlocal`，要注意变量的作用域
    - setlocal与endlocal之间的数据都是临时存在，出了endlocal区间就会清空,
    - 因为set %~2=%md5% 是在 setlocal 作用域内执行的，%~2 这个变量也只在局部环境中被赋值，endlocal一执行，它也就失效了,
    - **变量替换发生在整行之前，所以md5值仍在，只能在 endlocal & set 同一行中传值**

示例：

```bat
call :get_remote_md5 /tmp/dd.bin ret
if !errorlevel! (
    echo "error result"
    exit 0
)
exit 0
goto :eof

: 参数1为入参，参数2为出参
:get_remote_md5
    setlocal enabledelayedexpansion
    set "md5="
    set "LOCAL_FILE=%~1"
    for /f "tokens=1,* delims= " %%a in ('ssh !REMOTE! "md5sum !LOCAL_FILE!"') do (
        set "md5=%%a"
    )

    if defined md5 (
        endlocal & set "%~2=%md5%"
        exit /b 0
    ) else (
        endlocal & set "%~2="
        exit /b 1
    )
```
