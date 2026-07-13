@echo off
:: 切换到 UTF-8 代码页
chcp 65001 >nul
:: 关闭QuickEdit模式，解决程式執行緒直接被掛起
reg add HKCU\Console /v QuickEdit /t REG_DWORD /d 0 /f >nul
setlocal enabledelayedexpansion
set MYNAME=%~n0
set LOGFILE=%~dp0%~n0.log
set /a upgrade_suss_cnt=0
set /a upgrade_cnt=1
set "upgrade_soc_only=1"
set "FW_FILE=ota.bin"
:: Get the current working directory
set "current_dir=%cd%"
set "VERSION_FILE=%~dp0ota.version"
set "auto_run_enabled=1"
set "UPGRADED_FLAG=NO"
set /a AUTO_RUN_DELAY=5
set /a AUTO_REBOOT_DELAY_AFTER_UPGRADE=10

:hello_help
    set /p TARGET_VER=<"!VERSION_FILE!"
    cls
    echo ===============================================================
    echo   WELCOME TO USE FII OTA SCRIPT FOR WINDOW PLATFORM
    echo   - The default FW   : "!current_dir!\!FW_FILE!".
    echo   - The version file : "!VERSION_FILE!".
    echo   - The target VER   : "!TARGET_VER!"
    echo   - Notice that ONLY ONE TBox device should be CONNECTED by
    echo     WIFI, USB or EHTERNET way before upgrading.
    echo.
    echo   1. Clear the script log files ["!LOGFILE!"]
    echo   2. Review the script log files
    echo   3. Upgrade SOC + WNC
    echo   4. Upgrade SOC only
    echo   5. Select FW to upgrade
    echo   0. Exit
    echo ===============================================================
    echo.
    set "user_input="
    if "!auto_run_enabled!" == "1" (
        set "auto_run_enabled=0"
        echo The script will auto execute 4 upgrade SOC only after !AUTO_RUN_DELAY! seconds
        echo Press 'C' to CANCEL auto run.
        echo.
        choice /c C4 /d 4 /t !AUTO_RUN_DELAY! /m "Type the choice number: "
        if errorlevel 2 (
            set "user_input=4"
        ) else (
            echo Auto run canceled.
            goto :hello_help
        )
    ) else (
        set /p user_input="Type the choice number: "
    )

    if /i "!user_input!" == "1" (
        call :log_clr
        pause
        goto :hello_help
    )

    if /i "!user_input!" == "2" (
        type "!LOGFILE!"
        pause
        goto :hello_help
    )

    if /i "!user_input!" == "3" (
        set "upgrade_soc_only=0"
        call :start_upgrade
        goto :hello_help
    )

    if /i "!user_input!" == "4" (
        set "upgrade_soc_only=1"
        call :start_upgrade
        goto :hello_help
    )

    if /i "!user_input!" == "5" (
        call :update_fw_file
        goto :hello_help
    )

    if /i "!user_input!" == "0" (
        exit /b 0
    )
    echo Invalid choice!
    pause
    goto :hello_help


:start_upgrade
call :log "Current working directory: !current_dir!"
call :log "=== Start to upgrade here [!upgrade_cnt!]==="

:: Check if the file %FW_FILE% exists in the current directory
if exist "!FW_FILE!" (
    call :log "The file '!FW_FILE!' exists in the current directory."
) else (
    call :log "The file '!FW_FILE!' does not exist in the current directory. Please rename your OTA package to !FW_FILE!."
    pause
    goto :eof
)

set /a retry=0
:: Check device exist
:CHECK_DEVICE
call :log "check wheter a TBox is connected"
adb devices <nul > adb_devices.txt
set device_found=0
set device_type=unknown
set device_name=
for /f "skip=1 tokens=1" %%d in (adb_devices.txt) do (
    if not "%%d"=="" (
        set device_found=1
        set device_name=%%d
        echo %%d | findstr ":" >nul
        if !errorlevel!==0 (
            set device_type=IP
        ) else (
            set device_type=USB
        )
        goto :DEVICE_FOUND
    )
)

if "!device_found!"=="0" (
    set /a retry+=1
    if !retry! gtr 3 (
        goto :DEVICE_FOUND
    ) else if !retry! == 2 (
        call :log "Try kill adb server"
        adb kill-server < nul
    )
    ipconfig | find "192.168.225.1" >nul && (
        call :log "[!retry!] No device detected, try to connect to IP device 192.168.225.1:5555 with adb ..."
        adb connect 192.168.225.1:5555
    ) || (
        ipconfig | find "192.168.46.1" >nul && (
            call :log "[!retry!] No device detected, try to connect to IP device 192.168.46.1:5555 with adb ..."
            adb connect 192.168.46.1:5555
        ) || (
            goto :DEVICE_FOUND
        )
    )
    goto :CHECK_DEVICE
)

:DEVICE_FOUND
if "!device_found!"=="1" (
    set /a retry=0
    if "!device_type!"=="USB" (
        call :log "USB device detected: !device_name!"
        call :log "Current connection: USB"
    ) else if "!device_type!"=="IP" (
        call :log "IP device detected: !device_name!"
        call :log "Current connection: IP !device_name!"
        rem Already IP connection, no need to adb connect again
    ) else (
        call :log "Device detected: !device_name!"
    )
) else (
    call :log "No adb device found. Please check WIFI, USB, EHTERNET connection or device status."
    echo.
    set /p user_input="Type 'y' to try agian after device connected by manually or 'n' to exit? (y/n): "
    if /i "!user_input!" == "y" (
        goto :CHECK_DEVICE
    )
    goto :eof
)
del adb_devices.txt

:: adb login with password
call :login_device
if not !errorlevel! == 0 (
    goto :EXIT
)
call :log "Unlock success. login free now!"

adb shell "fii-mng.sh set Workmode 1" < nul

:: check current version info
call :log "Check current version information"
call :call_adbsh "fii-version --full | sed -n '1p'" CUR_VERSION

if not exist "!VERSION_FILE!" (
    call :log "Version file not found: !VERSION_FILE!"
    call :log "Please use menu 5 to select firmware first."
    pause
    goto :hello_help
)
set /p TARGET_VER=<"!VERSION_FILE!"

call :log "Target Version : !TARGET_VER!"
call :log "Current Version: !CUR_VERSION!"
if /i "!UPGRADED_FLAG!" == "DONE" (
    set "UPGRADED_FLAG=NO"
    if /i "!CUR_VERSION!"=="!TARGET_VER!" (
        set /a upgrade_suss_cnt+=1
        call :log "Version Verify PASS"
        call :log ""
        call :log "====Complete GOOD [!upgrade_cnt!] [!upgrade_suss_cnt!] [!dsn_str!]===="
        call :log ""
        set /a upgrade_cnt+=1
        set /p user_input="Connect to next TBOX for upgrading, type y if connected or quit? (y/n)"
        if /i "!user_input!" == "y" (
            call :log "=== Start to upgrade next TBOX [!upgrade_cnt!] ==="
            goto :CHECK_DEVICE
        )
        goto :hello_help
    ) else (
        call :log "Version Verify FAIL"
        call :log ""
        goto :ERROR_EXIT
    )
) else (
    if /i "!CUR_VERSION!"=="!TARGET_VER!" (
        call :log "Version same, no need to upgrade"
        choice /c yn /d n /t 5 /m "Force to upgrade?"
        if errorlevel 2 (
            call :log "End this upgrade loop"
            goto :hello_help
        )
        call :log "Still to upgrade by forcely"
    )
)
call :call_adbsh "fii-get-hwid" hwid_str
call :log "[fii-get-hwid]: !hwid_str!"
call :call_adbsh "fii-get-dsn" dsn_str
call :log "[fii-get-dsn]: !dsn_str!"

:: SYNC time as current PC
call :sync_time
if not !errorlevel! == 0 (
    goto :EXIT
)

:: Upload the OTA file
set "need_upload=1"
adb shell ls "/cache/!FW_FILE!" >nul 2>&1
if !errorlevel! == 0 (
    @REM for /f "tokens=1 delims= " %%a in ('busybox.exe md5sum %FW_FILE%') do (
    @REM     set "md5=%%a"
    @REM )
    for /f "delims=" %%i in ('CertUtil -hashfile "!FW_FILE!" MD5 ^| findstr /v ":"') do (
        set "md5=%%i"
    )
    for /f "tokens=1 delims= " %%a in ('adb shell md5sum /cache/!FW_FILE!') do (
        set "old_md5=%%a"
        for /f "delims=" %%i in ("!old_md5!") do set "old_md5=%%i"
    )
    if /i "!md5!" == "!old_md5!" (
        call :log "there is a same FW file in the device, skip upload step."
        set "need_upload=0"
    )  else (
        call :log "The firmware is different, cur:[!md5!]  device:[!old_md5!]"
    )
)
if !need_upload! == 1 (
    call :log "Begin to upload FW !FW_FILE! to device right now, waiting please"
    call :log "this step waste about more than 7 minutes by WIFI, press **ENTER** if no output for long time"
    adb push "!FW_FILE!" /cache > "tmp_err.log" 2>&1 < nul || (
        call :log "adb push failed"
        for /f "delims=" %%i in ('type tmp_err.log ^| findstr /v "^$"') do (
            call :log "%%i"
        )
        del tmp_err.log
        goto :EXIT
    )
    del tmp_err.log
    call :log "the FW file has been uploaded 100%."
)

:: Excute upgrade action
if "!upgrade_soc_only!" == "1" (
    goto :upgrade_soc
)
if "!upgrade_soc_only!" == "0" (
    goto :upgrade_all
)

goto :hello_help

:log_clr
    echo. > !LOGFILE!
    goto :eof

:log
    setlocal enabledelayedexpansion
    set "log_datetime=%date% %time:~0,8%"
    echo [!log_datetime!] [ota] %~1
    echo [!log_datetime!] [ota] %~1 >> !LOGFILE!
    endlocal
    goto :eof

:show_progress
    REM 参数1: 当前进度，参数2: 总数
    setlocal enabledelayedexpansion
    set "cur=%~1"
    set "tot=%~2"
    set /a percent=cur*100/tot
    set "bar="
    for /L %%i in (1,2,!cur!) do set "bar=!bar!="
    if not !cur! == !tot! (
        set "bar=!bar!>"
        for /L %%i in (!cur!,2,!tot!) do set "bar=!bar! "
    ) else (
        set "bar=!bar!=="
    )
    @REM <nul set /p="Progress: [!bar!] !percent!%%"
    echo Progress: [!bar!] !percent!%%
    endlocal & goto :eof

:get_unified_date
    REM arg1: return unified date
    :: 获取当前系统的 %date%
    setlocal enabledelayedexpansion

    where wmic >nul 2>&1 && (
        for /f "tokens=2 delims==" %%a in ('wmic os get localdatetime /value ^| find "="') do (
            set "datetime=%%a"
            set "unified_date=!datetime:~0,4!-!datetime:~4,2!-!datetime:~6,2!"
            for %%v in (!unified_date!) do endlocal & set "%~1=%%v"
            goto :eof
        )
    )

    where powershell >nul 2>&1 && (
        for /f "delims=" %%a in ('powershell -noprofile -command "Get-Date -Format 'yyyy-MM-dd'"') do (
            for %%v in (%%a) do endlocal & set "%~1=%%v"
            goto :eof
        )
    )
    call :log "error: cannot parse date: %date%"
    exit /b 1
    goto :eof

:sync_time
    REM 获取当前PC时间并同步到设备
    setlocal enabledelayedexpansion
    call :get_unified_date datestr
    echo %time%>nul
    set "HH=%time:~0,2%"
    set "MI=%time:~3,2%"
    set "SS=%time:~6,2%"
    if "%HH:~0,1%"==" " set "HH=0%HH:~1,1%"
    set "datestr=!datestr! %HH%:%MI%:%SS%"
    call :log "Sync PC time to device: %datestr%"
    set "result="
    for /f "delims=" %%i in ('adb shell "date -s \"%datestr%\" >/dev/null 2>&1; echo $?"') do (
        set "shell_exitcode=%%i"
        :: remove \r in shell_exitcode
        for /f "delims=" %%a in ("!shell_exitcode!") do set "shell_exitcode=%%a"
    )
    if "!shell_exitcode!" NEQ "0" (
        for /f "delims=" %%i in ('adb shell "date -s \"%datestr%\"" 2^>^&1') do (
            set "result=%%i"
        )
        call :log "sync time failed (code !shell_exitcode!): !result!"
        set ret=1
    ) else (
        call :log "sync success %datestr%"
        set ret=0
    )
    exit /b !ret!
    goto :eof

:login_device
    @REM avoid ! error in variable
    setlocal disabledelayedexpansion
    set "ret=0"
    set "pwd=Fvt:5g-tbox/@Szde99235W!!"
    for /f "delims=" %%i in ('adb shell "adb-login %pwd%" 2^>^&1') do (
        set login_result=%%i
    )
    setlocal enabledelayedexpansion
    echo !login_result! | findstr /C:"Adb: Unlocked" >nul
    if errorlevel 1 (
        call :log "Login Failed, device is locked, the reason: !login_result!"
        set "ret=1"
    ) else (
        set "ret=0"
    )
    :: assign ret to errorleverl
    set "retval=!ret!"
    endlocal & endlocal & exit /b %retval%
    goto :eof

:call_adbsh
    REM 参数1: cmd，参数2: return result
    setlocal enabledelayedexpansion
    if "%~1"=="" (
        call :log "error: less cmd arguments"
        exit /b 1
    )
    if "%~2"=="" (
        call :log "error: less return value"
        exit /b 1
    )
    set "shcmd=%~1"
    set "ret="

    :: 执行 adb shell 并过滤空行
    for /f "delims=" %%i in ('adb shell "%shcmd%" 2^>^&1 ^<nul ^| findstr /v "^$"') do (
        set "line=%%i"
        :: remove line end char \r if exist avoid echo error
        for /f "delims=" %%j in ("!line!") do (
            if "!ret!"=="" (
                set "ret=%%j"
            ) else (
                set "ret=!ret!; %%j"
            )
        )
    )
    if not defined ret (
        call :log "error: adb shell failed to execute: %shcmd%"
        exit /b 1
    )

    :: 去除末尾多余空格
    if "!ret:~-1!"==" " set "ret=!ret:~0,-1!"
    :: 将结果传递给调用方的变量（%~2）
    ::endlocal & set "%~2=%ret%"

    for %%v in ("!ret!") do (
        endlocal & set "%~2=%%~v"
    )
    exit /b 0

:upgrade_all
    call :log "Begin to upgrade now, execute fii-ota.sh FotaStartUpgrade /cache/%FW_FILE%"
    :: Start the upgrade and capture the output
    call :call_adbsh "fii-ota.sh FotaStartUpgrade /cache/%FW_FILE%" upgrade_result
    :: Check if the command executed successfully
    if errorlevel 1 (
        call :log "Cannot execute adb shell comand, Command output: !upgrade_result!"
        goto :EXIT
    )

    echo !upgrade_result! | findstr /C:"T-Box start updated" /C:"TBox is upgrading" >nul
    if errorlevel 1 (
        call :log "Start upgrade failed, Command output: !upgrade_result!"
        goto :ERROR_EXIT
    )

    :: Proceed to check upgrade progress
    call :log "it is upgrading, wait for the progress, press **ENTER** if no output for long time:"
    set "COMMAND=fii-ota.sh FotaGetProgress"
    set "progress_val=-1"

    :LOOP
        timeout /t 2 /nobreak >nul

        set "result="
        call :call_adbsh "%COMMAND%" result

        :: 检查命令是否执行成功
        if !errorlevel! neq 0 (
            call :log "failed to execute [ %COMMAND% ], error code:!errorlevel!"
            goto :ERROR_EXIT
        )

        :: 检查是否有结果
        if not defined result (
            call :log "there is no return result of [ %COMMAND% ], maybe failed to execute it"
            goto :ERROR_EXIT
        )

        echo !result! | findstr /C:"T-Box successfully updated" >nul
        if !errorlevel! == 0 (
            call :log "Upgrade success, Command output: !result!"
            goto :AFTER_UPGRADE
        )

        echo !result! | findstr /C:"T-Box update failed" >nul
        if !errorlevel! == 0 (
            call :log "Upgrade failed, Command output: !result!"
            goto :ERROR_EXIT
        )

        echo "!result!" | findstr /R /C:"[a-zA-Z]" >nul
        if !errorlevel! == 0 (
            call :log "Upgrade failed, Command output: [!result!]"
            goto :ERROR_EXIT
        )

        :: remove space before and after chars
        for /f "tokens=* delims= " %%a in ("!result!") do set "result=%%a"
        set /a result="!result:~0,-1!"
        if not "!progress_val!"=="!result!" (
            call :log "the progress is: !result!"
            call :show_progress !result! 100
            set progress_val=!result!
        )
        goto :LOOP

:ERROR_EXIT
    if "!upgrade_soc_only!"=="0" (
       adb shell "otaserver --logcat > /tmp/ota.log & sleep 3" <nul
       adb pull /tmp/ota.log . >nul
       call :log "---the ota failed log start with ))) ---"
       for /f "delims=" %%i in ('adb shell "tail -n 15 /tmp/ota.log"') do (
           set "LINE=%%i"
           call :log "))) !LINE!"
       )
        call :log "---the ota failed log end here ---"
    )
    call :log "====Complete FAIL [!upgrade_cnt!] [!upgrade_suss_cnt!] [!dsn_str!]===="
    set /a upgrade_cnt+=1
    pause
    goto :hello_help

:AFTER_UPGRADE
    choice /c yn /t !AUTO_REBOOT_DELAY_AFTER_UPGRADE! /d y /m "Do you want to reboot the 5g-tbox?(auto-select 'y' after !AUTO_REBOOT_DELAY_AFTER_UPGRADE!s)"
    if errorlevel 2 (
        set user_input=n
    ) else (
        set user_input=y
    )
    if /i "!user_input!" == "n" (
        call :log "====Complete INTERRUPT [!upgrade_cnt!] [!upgrade_suss_cnt!] [!dsn_str!]===="
        call :log "Go Home now?"
        pause
        goto :hello_help
    )
    if /i "!user_input!" == "y" (
        call :log "Reboot the 5g-tbox...."
        if "!upgrade_soc_only!" == "0" (
            adb shell fii-wnc-enforce-reboot.sh || (
                call :log "Reboot wnc failed"
                call :log "====Complete FAIL [!upgrade_cnt!] [!upgrade_suss_cnt!] [!dsn_str!]===="
                call :log ""
                set /a upgrade_cnt+=1
                pause
                goto :hello_help
            )
            timeout /t 1 /nobreak >nul
        )
        adb shell reboot || (
            call :log "Reboot soc failed"
            call :log "====Complete FAIL [!upgrade_cnt!] [!upgrade_suss_cnt!] [!dsn_str!]===="
            call :log ""
            set /a upgrade_cnt+=1
            pause
            goto :hello_help
        )
        adb disconnect >nul
        set /p user_input="Connect to TBOX for verifying, type y if connected or quit? (y/n)"
        if /i "!user_input!" == "y" (
            set "UPGRADED_FLAG=DONE"
            call :log "=== Start to verify the upgraded version ==="
            goto :CHECK_DEVICE
        )
    )
    pause
    goto :hello_help

:EXIT
    adb disconnect >nul
    call :log "this bat's log is saved as !LOGFILE!"
    call :log ""
    pause
    goto :hello_help


:upgrade_soc
    call :log "Begin to upgrade now, execute fii-upgrade-soc /cache/%FW_FILE%"
    call :log "Please wait for about 3 minutes, press **ENTER** if no output for long time"
    set "UPGRADE_PROGRESS=0"
    set "UPGRADE_RESULT=FAIL"
    for /f "delims=" %%i in ('adb shell ^<nul "fii-upgrade-soc /cache/%FW_FILE%"') do (
        set "LINE=%%i"

        call :log "!LINE!"

        echo !LINE! | findstr /i "100" >nul
        if not errorlevel 1 (
            set UPGRADE_PROGRESS=100
        )

        echo !LINE! | findstr /c:"SOC successfully upgraded" >nul
        if not errorlevel 1 (
            set UPGRADE_RESULT=PASS
        )
    )

    call :log "=========== Result=!UPGRADE_RESULT! ============"
    if "!UPGRADE_RESULT!_!UPGRADE_PROGRESS!" == "PASS_100" (
        goto :AFTER_UPGRADE
    )
    goto :ERROR_EXIT

:update_fw_file
    echo Input the absolute path of firmware or drag it in this console, then press 'ENTER' key
    set /p NFW_FILE=(default: "!current_dir!\!FW_FILE!"):

    if "!NFW_FILE!"=="" (
        call :log "Use the default"
    ) else (
        call :log "the FW file is !NFW_FILE!"

        if exist "!NFW_FILE!" (

            :: 获取纯文件名
            for %%F in ("!NFW_FILE!") do (
                set "FW_NAME=%%~nxF"
            )

            :: 清空
            set "TARGET_VER="
            :: 提取版本号
            for /f "tokens=1 delims=-" %%V in ("!FW_NAME:*R=!") do (
                set "TARGET_VER=R%%V"
            )

            echo !TARGET_VER!| findstr /r "^R[0-9][0-9][0-9]\.[0-9][0-9]*\.[0-9][0-9]*\.[A-Z][A-Z][A-Z]$" >nul
            if errorlevel 1 (
                call :log "Invalid firmware file: !FW_NAME!"
                call :log "Cannot extract version information."
                pause
                goto :eof
            )

            echo !TARGET_VER!> "!VERSION_FILE!"

            call :log "Target Version: !TARGET_VER!"

            copy /Y "!NFW_FILE!" "!current_dir!\!FW_FILE!"

            call :log "replace the FW successfully"

        ) else (
            call :log "the file is not exist, please check"
        )
    )

    pause
    goto :eof
