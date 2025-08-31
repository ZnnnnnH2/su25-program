#!/bin/bash

# 贪吃蛇AI日志分析工具

echo "🐍 编译贪吃蛇AI日志分析器..."

# 编译日志分析器
g++ -std=c++17 -O2 log_analyzer.cpp -o log_analyzer

if [ $? -eq 0 ]; then
    echo "✅ 编译成功!"
    echo ""
    
    # 如果有参数，直接分析文件
    if [ $# -gt 0 ]; then
        echo "📊 分析日志文件: $1"
        ./log_analyzer "$1"
    else
        echo "🎮 使用方法:"
        echo "1. 文件模式: ./analyze_logs.sh logfile.txt"
        echo "2. 交互模式: ./log_analyzer"
        echo "3. 单行分析: echo '|TICK:1|POS:10,5|...' | ./log_analyzer"
        echo ""
        echo "💡 示例日志格式:"
        echo "|TICK:1|POS:10,5|SCORE:100|LEN:3|SHIELD_CD:0|SHIELD_TIME:0|CAND:3|TARGET@12,8:SCORE:2.5,DIST:3,SNAKE_COST:0|NORMAL_MOVE:2"
        echo ""
        echo "🚀 启动交互模式..."
        ./log_analyzer
    fi
else
    echo "❌ 编译失败!"
    exit 1
fi
