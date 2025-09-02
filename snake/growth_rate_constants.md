# 增长率常量化改进报告

## 改进概述

将代码中所有与游戏增长率、阶段时间点、蛇长度阈值相关的硬编码数值替换为有意义的常量，提高代码的可维护性和可读性。

## 新增常量定义

### 1. 游戏阶段时间节点常量

```cpp
// 游戏阶段时间节点常量
static constexpr int EARLY_STAGE_END = 80;         // 早期阶段结束时间 (1-80回合)
static constexpr int MID_STAGE_END = 200;          // 中期阶段结束时间 (81-200回合)
static constexpr int LATE_STAGE_END = 256;         // 后期阶段结束时间 (201-256回合)
static constexpr int LATE_GAME_THRESHOLD = 160;    // 后期游戏阈值，用于策略调整
```

### 2. 食物生成率常量（基于官方游戏规则）

```cpp
// 早期阶段食物生成率 (1-80回合)
static constexpr double EARLY_REGULAR_FOOD_RATE = 0.80;  // 普通食物(1-3分): 80%
static constexpr double EARLY_GROWTH_FOOD_RATE = 0.20;   // 成长食物: 20%

// 中期阶段食物生成率 (81-200回合)
static constexpr double MID_REGULAR_FOOD_RATE = 0.78;    // 普通食物(1-5分): 78%
static constexpr double MID_GROWTH_FOOD_RATE = 0.12;     // 成长食物: 12%
static constexpr double MID_TRAP_RATE = 0.10;            // 陷阱: 10%

// 后期阶段食物生成率 (201-256回合)
static constexpr double LATE_REGULAR_FOOD_RATE = 0.80;   // 普通食物(1-5分): 80%
static constexpr double LATE_GROWTH_FOOD_RATE = 0.05;    // 成长食物: 5%
static constexpr double LATE_TRAP_RATE = 0.15;           // 陷阱: 15%
```

### 3. 蛇长度分类常量

```cpp
// 蛇长度相关常量
static constexpr int SMALL_SNAKE_THRESHOLD = 8;            // 小蛇长度阈值
static constexpr int MEDIUM_SNAKE_THRESHOLD = 12;          // 中型蛇长度阈值
static constexpr int MEDIUM_LARGE_SNAKE_THRESHOLD = 10;    // 中等大小蛇长度阈值
static constexpr int LARGE_SNAKE_THRESHOLD = 15;           // 大蛇长度阈值
static constexpr int VERY_LARGE_SNAKE_THRESHOLD = 20;      // 超大蛇长度阈值
static constexpr double SMALL_SNAKE_GROWTH_BONUS = 1.3;    // 小蛇成长奖励倍数(30%)
static constexpr double MEDIUM_SNAKE_GROWTH_BONUS = 1.1;   // 中型蛇成长奖励倍数(10%)
```

## 修改的函数和逻辑

### 1. calculate_growth_food_value() 函数

**修改前**:

```cpp
if (snake_length < 8)
    length_multiplier = 1.3; // 小蛇获得30%成长奖励
else if (snake_length < 12)
    length_multiplier = 1.1; // 中型蛇获得10%成长奖励
```

**修改后**:

```cpp
if (snake_length < SMALL_SNAKE_THRESHOLD)
    length_multiplier = SMALL_SNAKE_GROWTH_BONUS; // 小蛇获得成长奖励
else if (snake_length < MEDIUM_SNAKE_THRESHOLD)
    length_multiplier = MEDIUM_SNAKE_GROWTH_BONUS; // 中型蛇获得成长奖励
```

### 2. 游戏时间计算

**修改前**:

```cpp
s.current_ticks = 256 - s.remaining_ticks;
```

**修改后**:

```cpp
s.current_ticks = LATE_STAGE_END - s.remaining_ticks;
```

### 3. 蛇长度策略判断

在多个位置替换了硬编码的蛇长度阈值：

- `me.length >= 15` → `me.length >= LARGE_SNAKE_THRESHOLD`
- `me.length >= 20` → `me.length >= VERY_LARGE_SNAKE_THRESHOLD`
- `me.length >= 10` → `me.length >= MEDIUM_LARGE_SNAKE_THRESHOLD`

### 4. 后期游戏策略调整

**修改前**:

```cpp
if (global_state.current_ticks > 160) // Late game
```

**修改后**:

```cpp
if (global_state.current_ticks > LATE_GAME_THRESHOLD) // Late game
```

## 改进效果

### 1. **代码可维护性提升**

- 所有关键数值都集中在常量定义区域
- 修改游戏参数时只需要在一个地方修改
- 常量名称具有自解释性

### 2. **代码可读性增强**

- 替换了晦涩的数字硬编码
- 每个常量都有清晰的注释说明
- 逻辑更容易理解和验证

### 3. **参数调优便利性**

- 可以轻松调整蛇长度分类阈值
- 可以方便地修改游戏阶段时间点
- 便于进行 A/B 测试和参数优化

### 4. **基于官方规则的准确性**

- 食物生成率常量严格基于官方游戏规则
- 确保 AI 策略与实际游戏机制一致
- 便于根据规则变化进行调整

## 潜在的未来改进

### 1. 配置文件支持

可以考虑将这些常量移到配置文件中，实现运行时调整：

```cpp
// 可以从配置文件读取的常量
extern int EARLY_STAGE_END;
extern int MID_STAGE_END;
// ...
```

### 2. 动态阈值计算

某些阈值可以基于当前游戏状态动态计算：

```cpp
// 基于存活蛇数量的动态阈值
int get_large_snake_threshold(int alive_snakes) {
    return LARGE_SNAKE_THRESHOLD + (alive_snakes - 1) * 2;
}
```

### 3. 编译时优化

使用 constexpr 函数进行编译时计算：

```cpp
constexpr double calculate_stage_bonus(int stage) {
    return stage == 1 ? EARLY_GROWTH_FOOD_RATE :
           stage == 2 ? MID_GROWTH_FOOD_RATE :
                        LATE_GROWTH_FOOD_RATE;
}
```

## 总结

通过将增长率和相关阈值编码为常量，代码的质量得到了显著提升。这种改进不仅使代码更加专业和规范，也为未来的功能扩展和参数调优奠定了良好的基础。所有常量都基于官方游戏规则，确保了 AI 策略的准确性和有效性。
