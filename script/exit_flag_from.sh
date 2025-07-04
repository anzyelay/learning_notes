#!/bin/bash

# === 配置参数（可通过变量修改） ===
TIMEOUT=${TIMEOUT:-10}  #超时时间（秒） 优先读取环境变量，未设置时默认10秒
EXIT_CODE=1       # 退出码（超时后退出状态码）
CHECK_INTERVAL=${CHECK_INTERVAL:-1}  # 时间检查间隔（秒）
MAIN_PID=$$
# ==================================

START_TIME=$(date +%s)
EXIT_FLAG=0       # 退出标志（0=继续运行，1=需要退出）

# 定义定时显示时间信息并检查超时的函数
display_time_and_check_timeout() {
  while [ $EXIT_FLAG -eq 0 ]; do
    CURRENT_TIME=$(date +%s)
    ELAPSED=$((CURRENT_TIME - START_TIME))

    # 显示当前时间
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] 脚本运行中..."

    # 检查是否超时
    if [ $ELAPSED -ge $TIMEOUT ]; then
      echo "错误：超时（${TIMEOUT}秒），终止脚本！"
      kill -SIGUSR1 $MAIN_PID
      exit $EXIT_CODE
    fi

    sleep $CHECK_INTERVAL
  done
}

trap 'echo "收到超时信号，主任务退出"; EXIT_FLAG=1' SIGUSR1
# 启动定时检查线程（后台运行）
display_time_and_check_timeout &
TIME_PID=$!

# 主任务（示例：无限循环）
echo "主任务启动..."
while [ $EXIT_FLAG -eq 0 ]; do
  # === 这里放置实际任务逻辑 ===
  echo "主任务正在执行..."
  sleep 2  # 模拟任务耗时
  # ==============================

  # 可选：通过其他条件提前退出
  # if [ 某些条件 ]; then
  #   EXIT_FLAG=1
  # fi
done

# 清理资源（如果需要）
echo "正在清理资源..."
kill $TIME_PID 2>/dev/null
wait $TIME_PID 2>/dev/null
echo "脚本正常退出"
exit 0
