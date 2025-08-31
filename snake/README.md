# 🐍 贪吃蛇AI日志分析器

这个工具可以详细分析贪吃蛇AI的决策日志，帮助理解AI的思考过程和决策逻辑。

## 📁 文件说明

- `my-snake.cpp` - 增强后的贪吃蛇AI，包含详细的日志记录
- `log_analyzer.cpp` - 日志分析器源码
- `analyze_logs.sh` - 编译和运行脚本
- `sample_log.txt` - 示例日志文件

## 🚀 使用方法

### 1. 编译和测试
```bash
# 给脚本执行权限（首次使用）
chmod +x analyze_logs.sh

# 编译并测试示例日志
./analyze_logs.sh sample_log.txt
```

### 2. 分析真实游戏日志
```bash
# 运行贪吃蛇AI并保存日志
./my-snake < input.txt > output.txt 2>&1

# 分析日志（假设日志在output.txt的第二行）
tail -n +2 output.txt | ./log_analyzer
```

### 3. 交互模式
```bash
# 启动交互模式
./log_analyzer

# 然后输入日志行，例如：
|TICK:1|POS:15,20|SCORE:0|LEN:3|SHIELD_CD:0|SHIELD_TIME:0|CAND:2|TARGET@10,15:SCORE:1.1,DIST:8,SNAKE_COST:0|NORMAL_MOVE:1
```

## 📊 日志格式说明

AI现在会生成详细的结构化日志，包含以下信息：

### 基本状态信息
- `TICK:n` - 当前回合数
- `POS:y,x` - 蛇头位置坐标
- `SCORE:n` - 当前分数
- `LEN:n` - 蛇身长度
- `SHIELD_CD:n` - 护盾冷却时间
- `SHIELD_TIME:n` - 护盾持续时间

### 目标分析信息
- `CAND:n` - 发现的候选目标数量
- `ITEM_type@y,x:Vvalue,Ddist,SCscore` - 考虑的物品信息
- `CHEST@y,x:...` - 考虑的宝箱信息
- `TARGET@y,x:SCORE:s,DIST:d,SNAKE_COST:c` - 选择的目标详情

### 决策类型
- `NORMAL_MOVE:action` - 正常移动决策
- `SURVIVAL_MODE` - 进入生存模式
- `SURVIVAL_MOVE:action,REACH:n` - 生存模式移动
- `SURVIVAL_SHIELD` - 生存模式开启护盾
- `DESPERATE_MOVE:action` - 绝望移动
- `FINAL_SHIELD` - 最终护盾尝试
- `SNAKE_PASS_WITH_SHIELD:action` - 使用护盾穿越
- `SNAKE_DETECTED:ACTIVATING_SHIELD` - 检测到蛇身，激活护盾
- `DANGER_DETECTED:ACTIVATING_SHIELD` - 检测到危险，激活护盾

### 错误信息
- `ERROR:TARGET_UNREACHABLE` - 目标不可达
- `ERROR:NO_DIRECTION_FOUND` - 无法找到移动方向

## 🎯 分析报告内容

分析器会生成包含以下内容的详细报告：

1. **📍 位置状态** - 当前坐标、分数、长度
2. **🛡️ 护盾状态** - 护盾可用性、冷却时间、持续时间
3. **🎯 目标分析** - 候选目标、考虑的物品、选择的目标
4. **🧠 决策分析** - 决策类型和策略解释
5. **⚡ 最终行动** - 执行的动作
6. **💭 决策原因** - 详细的决策reasoning

## 🎮 决策类型说明

- **🎯 正常目标追求** - AI找到有价值目标并规划安全路径
- **🆘 生存模式** - 没有合适目标，AI进入求生状态
- **⚠️ 绝望移动** - 情况危急，AI忽略部分安全检查
- **🚨 最终手段** - 所有移动都不可行，强制尝试护盾
- **🛡️ 护盾穿越** - 利用护盾穿过敌方蛇身
- **❌ 错误处理** - 算法遇到异常，启动恢复机制

## 🔧 自定义扩展

如果你想添加更多分析功能，可以修改：

1. `log_analyzer.cpp` 中的 `parse_log_line()` 函数来解析新的日志字段
2. `print_analysis()` 函数来添加新的分析内容
3. `my-snake.cpp` 中的日志输出来记录更多信息

## 💡 使用技巧

1. **实时分析**: 可以在游戏运行时实时分析日志
2. **性能调优**: 通过分析日志找出AI决策的瓶颈
3. **策略优化**: 观察AI在不同情况下的决策模式
4. **调试助手**: 快速定位AI行为异常的原因

享受分析你的AI决策过程吧！🐍✨
