#include <bits/stdc++.h>
using namespace std;

// ==================== 游戏常量定义 ====================
// 地图尺寸定义：游戏地图的宽度和高度
static constexpr int W = 40;            // 地图宽度 (x坐标最大值，范围：0到39)
static constexpr int H = 30;            // 地图高度 (y坐标最大值，范围：0到29)
static constexpr int MYID = 2024201540; // 玩家学号：用于标识自己的蛇身体

// ==================== 护盾和移动代价常量 ====================
// 用于BFS路径规划中计算通过蛇身的代价
static constexpr int SNAKE_COST_WEIGHT = 10;     // 蛇身代价在总代价中的权重倍数
static constexpr int SNAKE_COST_NO_SHIELD = 100; // 无护盾时穿过蛇身的高代价（强烈不推荐）
static constexpr int SNAKE_COST_WITH_SHIELD = 0; // 有护盾时穿过蛇身的代价（无惩罚）
static constexpr int SNAKE_COST_OPEN_SHIELD = 2; // 使用护盾来穿过蛇身的额外代价
static constexpr int SHIELD_COST_THRESHOLD = 20; // 使用护盾所需的最低分数门槛
static constexpr int TRAP_STEP_COST = 30;        // 陷阱步骤惩罚代价，用于路径规划中软性避开陷阱

// ==================== 食物和物品价值常量 ====================
// 定义游戏中各种物品的价值，用于目标选择的决策算法
static constexpr int GROWTH_FOOD_VALUE = 8;      // 成长食物价值（使蛇身变长的特殊食物）
static constexpr int TRAP_PENALTY = -10;         // 陷阱扣分（负值，踩中会被扣分）
static constexpr int KEY_VALUE = 75;             // 钥匙价值（用于开启宝箱）
static constexpr int CHEST_VALUE = 100;          // 宝箱基础价值（需要钥匙才能开启）
static constexpr int NORMAL_FOOD_MULTIPLIER = 4; // 普通食物价值倍数（实际价值=食物类型值×此倍数）

// ==================== 评分和权重常量 ====================
// 用于目标评分算法的各种权重和修正因子
static constexpr double SNAKE_SAFETY_PENALTY_RATE = 0.5; // 蛇身穿越安全惩罚率（每个蛇身格子的惩罚倍数）
static constexpr double CHEST_SCORE_MULTIPLIER = 2.0;    // 宝箱评分倍数（宝箱价值更高）
static constexpr double DEFAULT_CHEST_SCORE = 60.0;      // 默认宝箱分数（当宝箱分数<=0时使用的备用值）
static constexpr double DISTANCE_OFFSET = 1.0;           // 距离计算偏移量，避免除零错误
static constexpr int SCORE_DISPLAY_MULTIPLIER = 100;     // 分数显示时的放大倍数（用于日志输出，便于观察）
static constexpr double LIFETIME_SOFT_DECAY = 0.9;       // 寿命衰减底数（每多1步到达，乘以此因子，体现紧迫性）
static constexpr double CONTEST_PENALTY = 0.5;           // 竞争目标惩罚系数（被对手同样或更快到达的目标评分乘以此值）
static constexpr double NEXTZONE_RISK_PENALTY = 0.35;    // 缩圈风险惩罚（缩圈前无法抵达且目标不在下个安全区时的折扣）
static constexpr double DEGREE_BONUS = 0.06;             // 局部自由度奖励系数（每个安全邻居给予的乘数奖励，避免死胡同）
static constexpr double MORE_FOOD_DIRECTION = 0.1;       // 更多食物方向奖励系数（朝向食物的移动给予额外奖励）
// ==================== 前瞻算法常量 ====================
// 控制AI的前瞻深度：更深的前瞻可以找到更优路径，但计算开销更大
static constexpr int LOOKAHEAD_DEPTH = 1; // 1=启用一步前瞻算法；0=关闭前瞻，仅用贪心策略

// ==================== 风险评估常量 ====================
// 用于各种风险情况的检测阈值
static constexpr int SAFE_ZONE_BOUNDARY_THRESHOLD = 2;   // 安全区边界危险邻近阈值（距离边界多近算危险）
static constexpr int SAFE_ZONE_SHRINK_THRESHOLD = 4;     // 安全区收缩危险阈值（距离收缩多近需要警惕）
static constexpr int ENEMY_BODY_PROXIMITY_THRESHOLD = 2; // 敌蛇身体危险邻近阈值（距离敌蛇多近算危险）
static constexpr int TRAP_PROXIMITY_THRESHOLD = 2;       // 陷阱危险邻近阈值（距离陷阱多近需要小心）

// ==================== 数据结构定义 ====================
// 与游戏引擎格式对齐的结构体

/**
 * 坐标点结构
 * 遵循游戏引擎的坐标系统：y为行（垂直方向），x为列（水平方向）
 */
struct Point
{
    int y, x; // y: 行坐标(0-29), x: 列坐标(0-39)
};

/**
 * 游戏物品结构
 * 存储地图上各种可交互物品的信息
 *
 * type和value的对应关系：
 * - type=1到5: 普通食物，value=type*4（得分）
 * - type=-1: 成长食物（增长蛇身），value=8
 * - type=-2: 陷阱（有害），value=-10（扣分）
 * - type=-3: 钥匙（开宝箱用），value=50
 * - type=-5: 宝箱（需钥匙），value=100
 */
struct Item
{
    Point pos;    // 物品在地图上的位置坐标
    int type;     // 物品类型（见上方说明）
    int value;    // 物品价值（正数为得分，负数为扣分）
    int lifetime; // 物品剩余生存时间（-1表示永久存在，>0表示剩余回合数）
};

/**
 * 蛇的完整状态结构
 * 包含蛇的所有属性和状态信息
 */
struct Snake
{
    int id, length, score, dir;                        // 蛇的唯一ID、当前长度、得分、移动方向(0-3)
    int shield_cd, shield_time;                        // 护盾冷却时间、护盾剩余持续时间
    bool has_key = false;                              // 是否持有钥匙（用于开启宝箱）
    vector<Point> body;                                // 蛇身体坐标列表（索引0为头部，后续为身体）
    const Point &head() const { return body.front(); } // 便捷方法：获取蛇头位置
};

/**
 * 宝箱结构
 * 需要钥匙才能开启的高价值目标
 */
struct Chest
{
    Point pos; // 宝箱在地图上的位置
    int score; // 宝箱内的分数奖励
};

/**
 * 钥匙结构
 * 用于开启宝箱，每个钥匙有使用期限
 */
struct Key
{
    Point pos;          // 钥匙位置（如果在地上）
    int holder_id;      // 持有者蛇的ID (-1表示钥匙在地上无人持有)
    int remaining_time; // 钥匙剩余有效时间（到期后消失）
};

/**
 * 安全区域边界结构
 * 定义矩形安全区域的四个边界坐标
 */
struct Safe
{
    int x_min, y_min, x_max, y_max; // 安全区域的边界坐标（包含边界点）
};

// 构建地图：使用二维数组直接存储地图状态，替代原来的位掩码系统
array<array<int, W>, H> mp;

/**
 * 游戏状态结构
 * 包含一个回合中完整的游戏状态信息，是AI决策的主要数据来源
 */
struct State
{
    int current_ticks;                 // 当前已经过的回合数
    int remaining_ticks;               // 游戏剩余回合数
    vector<Item> items;                // 地图上的所有物品（食物、陷阱、钥匙等）
    vector<Snake> snakes;              // 游戏中的所有蛇（包括自己和对手）
    vector<Chest> chests;              // 地图上的所有宝箱
    vector<Key> keys;                  // 地图上的所有钥匙（包括被持有的）
    vector<Point> traps;               // 地图上的所有陷阱位置（从items中提取）
    Safe cur, next, fin;               // 当前安全区、下次收缩后的安全区、最终安全区
    int next_tick = -1, fin_tick = -1; // 安全区缩小的时间点（-1表示不会缩小）
    int self_idx = -1;                 // 自己蛇在snakes数组中的索引

    // 获取自己蛇的便捷方法
    const Snake &self() const { return snakes[self_idx]; }
};

string str_info;    // 存储每回合的决策日志信息，用于调试和分析
State global_state; // 全局游戏状态，存储当前回合的完整游戏信息

// ==================== 输入输出：每回合读取游戏状态 ====================

/**
 * 从标准输入读取游戏状态（符合游戏引擎API格式）
 *
 * 输入格式说明：
 * 1. 剩余回合数
 * 2. 物品数量及每个物品的详细信息
 * 3. 蛇的数量及每条蛇的详细信息
 * 4. 宝箱数量及每个宝箱的详细信息
 * 5. 钥匙数量及每个钥匙的详细信息
 * 6. 安全区域信息（当前、下次、最终）
 *
 * @param s 要填充的游戏状态结构
 */
static void read_state(State &s)
{

    // 读取剩余回合数，如果读取失败则程序退出
    if (!(cin >> s.remaining_ticks))
        exit(0);
    s.current_ticks = 256 - s.remaining_ticks; // 计算当前已进行的回合数（总回合256）

    // ========== 读取物品信息 ==========
    int m;
    cin >> m; // 物品数量
    s.items.resize(m);
    for (int i = 0; i < m; i++)
    {
        cin >> s.items[i].pos.y >> s.items[i].pos.x >> s.items[i].type >> s.items[i].lifetime;
        // 根据物品类型设置对应的价值
        switch (s.items[i].type)
        {
        case -1: // 成长食物：使蛇身变长
            s.items[i].value = GROWTH_FOOD_VALUE;
            break;
        case -2: // 陷阱：踩中会扣分
            s.items[i].value = TRAP_PENALTY;
            break;
        case -3: // 钥匙：用于开启宝箱
            s.items[i].value = KEY_VALUE;
            break;
        case -5: // 宝箱：高价值目标，需要钥匙
            s.items[i].value = CHEST_VALUE;
            break;
        default: // 普通食物：类型值1-5，价值为类型值乘以倍数
            s.items[i].value = s.items[i].type * NORMAL_FOOD_MULTIPLIER;
            break;
        }
    }

    // ========== 读取蛇信息 ==========
    int ns;
    cin >> ns; // 蛇的数量
    s.snakes.resize(ns);
    unordered_map<int, int> id2idx; // 蛇ID到数组索引的映射，用于快速查找
    id2idx.reserve(ns * 2);
    for (int i = 0; i < ns; i++)
    {
        auto &sn = s.snakes[i];
        cin >> sn.id >> sn.length >> sn.score >> sn.dir >> sn.shield_cd >> sn.shield_time;
        sn.body.resize(sn.length);
        for (int j = 0; j < sn.length; j++)
            cin >> sn.body[j].y >> sn.body[j].x;
        // 识别自己的蛇
        if (sn.id == MYID)
            s.self_idx = i; // 记录自己蛇在数组中的索引
        id2idx[sn.id] = i;
    }

    // 检查是否找到自己的蛇（安全性检查）
    if (s.self_idx == -1)
    {
        // 如果没找到自己的蛇，可能已经死亡，输出默认动作并退出
        cout << "0\n";
        cout << "|ERR:SNAKE_NOT_FOUND\n";
        exit(0);
    }

    // ========== 读取宝箱信息 ==========
    int nc;
    cin >> nc; // 宝箱数量
    s.chests.resize(nc);
    for (int i = 0; i < nc; i++)
        cin >> s.chests[i].pos.y >> s.chests[i].pos.x >> s.chests[i].score;

    // ========== 读取钥匙信息 ==========
    int nk;
    cin >> nk; // 钥匙数量
    s.keys.resize(nk);
    for (int i = 0; i < nk; i++)
    {
        cin >> s.keys[i].pos.y >> s.keys[i].pos.x >> s.keys[i].holder_id >> s.keys[i].remaining_time;
        // 如果钥匙被某条蛇持有，标记该蛇的has_key属性
        if (s.keys[i].holder_id != -1)
        {
            auto it = id2idx.find(s.keys[i].holder_id);
            if (it != id2idx.end())
                s.snakes[it->second].has_key = true;
        }
    }

    // ========== 读取安全区域信息 ==========
    // 当前安全区边界
    cin >> s.cur.x_min >> s.cur.y_min >> s.cur.x_max >> s.cur.y_max;
    // 下次安全区收缩的时间和收缩后的边界
    cin >> s.next_tick >> s.next.x_min >> s.next.y_min >> s.next.x_max >> s.next.y_max;
    // 最终安全区的时间和边界
    cin >> s.fin_tick >> s.fin.x_min >> s.fin.y_min >> s.fin.x_max >> s.fin.y_max;

    // 可选的调试信息行被忽略（如果有的话）
    // cin >> str_info;
}

// ==================== 辅助函数 ====================

/**
 * 检查坐标是否在地图边界内
 * @param y 行坐标
 * @param x 列坐标
 * @return 如果坐标在地图范围内返回true，否则返回false
 */
inline bool in_bounds(int y, int x)
{
    return (0 <= y && y < H && 0 <= x && x < W);
}

/**
 * 检查坐标是否在指定安全区域内（不包括地图边界检查）
 * @param z 安全区域结构
 * @param y 行坐标
 * @param x 列坐标
 * @return 如果坐标在安全区内返回true，否则返回false
 */
inline bool in_safe_zone(const Safe &z, int y, int x)
{
    return x >= z.x_min && x <= z.x_max && y >= z.y_min && y <= z.y_max;
}

/**
 * 若坐标在当前安全区 z 内：
 *   1) 如果下次收缩不存在 (next_tick == -1) 或 距离收缩还有 >5 回合，返回 0
 *   2) 否则返回该点到“收缩后安全区”(global_state.next) 的曼哈顿距离（若点已在下一安全区内则为0）
 * 若坐标不在当前安全区内，返回 -1
 */
inline int danger_safe_zone(const Safe &z, int y, int x)
{
    // 不在当前安全区
    if (!(x >= z.x_min && x <= z.x_max && y >= z.y_min && y <= z.y_max))
        return -1;

    // 下次收缩信息
    const int next_tick = global_state.next_tick;
    const int now_tick = global_state.current_ticks;

    // 不会在 5 回合内收缩
    if (next_tick == -1 || (next_tick - now_tick) > 5)
        return 0;

    // 计算到收缩后安全区的曼哈顿“外距”（在内部为0）
    const Safe &nz = global_state.next;
    int dx = 0, dy = 0;
    if (x < nz.x_min)
        dx = nz.x_min - x;
    else if (x > nz.x_max)
        dx = x - nz.x_max;
    if (y < nz.y_min)
        dy = nz.y_min - y;
    else if (y > nz.y_max)
        dy = y - nz.y_max;
    return dx + dy;
}

// ==================== 移动方向映射 ====================
// 游戏引擎动作映射 (按API规范): 0=左,1=上,2=右,3=下,4=护盾
static const int DX[4] = {-1, 0, 1, 0}; // x方向偏移：左(-1)、上(0)、右(+1)、下(0)
static const int DY[4] = {0, -1, 0, 1}; // y方向偏移：左(0)、上(-1)、右(0)、下(+1)
static const int ACT[4] = {0, 1, 2, 3}; // 方向索引到动作代码的映射

// ==================== 地图构建 ====================

/**
 * 构建完整游戏地图
 * 将所有游戏对象映射到二维数组中，替代原来的位掩码系统
 *
 * 地图值定义（遵循游戏API规范）：
 * - 0: 安全区外（蛇无法生存的区域）
 * - 1-5: 普通食物（数字表示食物类型和基础分值）
 * - -1: 增长豆（使蛇身变长的特殊食物）
 * - -2: 陷阱（踩中会扣分的危险区域）
 * - -3: 钥匙（用于开启宝箱）
 * - -5: 宝箱（需要钥匙才能开启的高价值目标）
 * - 学号(>1000): 蛇身体（用蛇的ID标识）
 * - 999999: 空白安全区域（可以安全移动的空地）
 *
 * @param s 当前游戏状态
 */
static void build_map(const State &s)
{
    // 第一步：初始化地图基础状态
    // 安全区内设为999999（空白可移动区域），安全区外设为0（危险区域）
    for (int y = 0; y < H; y++)
    {
        for (int x = 0; x < W; x++)
        {
            if (in_safe_zone(s.cur, y, x))
                mp[y][x] = 999999; // 标记为空白安全区域
            else
                mp[y][x] = 0; // 标记为安全区外
        }
    }

    // 第二步：放置地图上的物品（食物、陷阱、钥匙等）
    for (const auto &item : s.items)
    {
        int y = item.pos.y, x = item.pos.x;
        if (in_bounds(y, x) && in_safe_zone(s.cur, y, x))
        {
            mp[y][x] = item.type; // 直接使用API定义的物品类型值
        }
    }

    // 第三步：放置宝箱（覆盖之前的值，因为宝箱优先级较高）
    for (const auto &chest : s.chests)
    {
        int y = chest.pos.y, x = chest.pos.x;
        if (in_bounds(y, x) && in_safe_zone(s.cur, y, x))
        {
            mp[y][x] = -5; // 宝箱统一标记为-5
        }
    }

    // 第四步：放置在地上的钥匙（未被蛇持有的钥匙）
    for (const auto &key : s.keys)
    {
        if (key.holder_id == -1) // 钥匙在地上，没有被任何蛇持有
        {
            int y = key.pos.y, x = key.pos.x;
            if (in_bounds(y, x) && in_safe_zone(s.cur, y, x))
            {
                mp[y][x] = -3; // 钥匙标记为-3
            }
        }
    }

    // 第五步：放置所有蛇的身体（最高优先级，会覆盖其他物品）
    for (const auto &snake : s.snakes)
    {
        for (const auto &body_part : snake.body)
        {
            int y = body_part.y, x = body_part.x;
            if (in_bounds(y, x) && in_safe_zone(s.cur, y, x))
            {
                mp[y][x] = snake.id; // 使用蛇的学号标识
            }
        }
    }
}

// ==================== 地图查询辅助函数 ====================
// 提供各种地图状态查询功能，简化复杂的地图判断逻辑

/**
 * 检查位置是否被阻挡（不能通过）
 *
 * 阻挡条件：
 * 1. 超出地图边界或在安全区外
 * 2. 其他蛇的身体（自己的蛇身除外）
 * 3. 没有钥匙时遇到宝箱
 *
 * @param y 行坐标
 * @param x 列坐标
 * @param has_key 是否持有钥匙（影响宝箱通过性）
 * @return 如果位置被阻挡返回true，否则返回false
 */
inline bool unsafe_to_go(int y, int x, bool has_key = false)
{
    if (!in_bounds(y, x) || mp[y][x] == 0) // 边界外或安全区外
        return true;

    int val = mp[y][x];

    // 其他蛇的身体（自己的蛇身除外，学号都大于1000）
    if (val > 1000 && val != MYID)
        return true;

    // 宝箱（如果没有钥匙则无法通过）
    if (val == -5 && !has_key)
        return true;

    return false;
}

/**
 * 检查位置是否是蛇身体（包括自己和其他蛇）
 * @param y 行坐标
 * @param x 列坐标
 * @return 如果是任何蛇的身体not indcude self返回true，否则返回false
 */
inline bool go_but_need_shield(int y, int x)
{
    if (mp[y][x] == 0)
    {
        return true;
    }

    int val = mp[y][x];
    return val > 1000 && val != MYID; // 学号都大于1000，用于标识蛇身
}

/**
 * 检查位置是否是陷阱
 * @param y 行坐标
 * @param x 列坐标
 * @return 如果是陷阱返回true，否则返回false
 */
inline bool is_trap(int y, int x)
{
    return mp[y][x] == -2; // 陷阱的标识值
}

/**
 * 检查位置是否是食物或其他可获取物品
 * 包括：普通食物(1-5)、增长豆(-1)、钥匙(-3)、宝箱(-5)
 * @param y 行坐标
 * @param x 列坐标
 * @return 如果是可获取物品返回true，否则返回false
 */
inline bool is_food_or_item(int y, int x)
{
    if (!in_bounds(y, x) || mp[y][x] == 0)
        return false;

    int val = mp[y][x];
    return (val >= 1) || val == -1 || val == -3 || (val == -5 && global_state.self().has_key); // 普通食物、增长豆、钥匙
}

/**
 * 检查位置对所有蛇的价值
 * >0 有价值
 * =0 自己蛇身
 * -1 需要躲避的
 * @param y 行坐标
 * @param x 列坐标
 * @return 如果是宝箱返回true，否则返回false
 */
inline int is_valuable(int index_of_snake, int y, int x)
{
    if (!in_bounds(y, x) || mp[y][x] == 0)
        return -1;

    if (mp[y][x] > 1000)
    {
        return (mp[y][x] == global_state.snakes[index_of_snake].id) ? 1 : -1;
    }

    if (mp[y][x] >= 1)
    {
        return mp[y][x] + 1; // 普通食物，返回其类型值
    }

    if (mp[y][x] == -1)
    {
        return GROWTH_FOOD_VALUE; // 增长豆，返回其固定价值
    }

    if (mp[y][x] == -2)
    { // 陷阱
        return -1;
    }

    if (mp[y][x] == -3)
    {
        return 100; // 钥匙
    }

    if (mp[y][x] == -5)
    {
        return 100; // 宝箱
    }

    return 1;
}

/**
 * 检查位置是否危险（敌蛇头部附近）
 * 危险区域定义：距离任何敌蛇头部曼哈顿距离为1的位置
 * @param y 行坐标
 * @param x 列坐标
 * @param s 游戏状态（用于获取敌蛇位置）
 * @return 如果位置危险返回true，否则返回false
 */
inline bool is_danger_zone(int y, int x, const State &s)
{
    if (!in_bounds(y, x) || mp[y][x] == 0)
        return true; // 边界外或安全区外都是危险的

    // 检查是否在任何敌蛇头部的相邻位置
    for (const auto &snake : s.snakes)
    {
        if (snake.id == MYID)
            continue; // 跳过自己

        auto head = snake.head();
        int dist = abs(y - head.y) + abs(x - head.x);
        if (dist == 1) // 曼哈顿距离为1（相邻位置）
            return true;
    }

    return false;
}

/**
 * 检查是否可以开启护盾
 * 开启护盾的条件：护盾冷却完毕且分数足够
 * @return 如果可以开启护盾返回true，否则返回false
 */
inline bool can_open_shield()
{
    if (global_state.self().shield_cd == 0 && global_state.self().score >= SHIELD_COST_THRESHOLD)
        return true;
    return false;
}

// ==================== 广度优先搜索 (BFS) ====================

/**
 * BFS算法输出结构
 * 存储从起点到各个位置的最短路径信息
 */
struct BFSOut
{
    array<array<int, W>, H> dist;       // 距离矩阵：dist[y][x] = 从起点到(y,x)的最短距离
    array<array<int, W>, H> snake_cost; // 蛇身代价矩阵：snake_cost[y][x] = 到达(y,x)穿过的蛇身格子数
    array<array<int, W>, H> parent;     // 父节点方向矩阵：parent[y][x] = 到达(y,x)的来源方向
};

/**
 * 在网格上执行BFS，从起始位置(sy, sx)开始计算到所有可达位置的最短距离和路径
 *
 * 算法特点：
 * 1. 使用优先队列实现带权重的搜索，优先选择代价低的路径
 * 2. 考虑蛇身穿越的额外代价，有护盾时代价较低
 * 3. 对陷阱施加额外惩罚，但不完全禁止通过
 * 4. 支持护盾机制：有护盾可穿过蛇身，无护盾代价极高
 *
 * @param s 游戏状态
 * @param sy 起始y坐标
 * @param sx 起始x坐标
 * @return BFS结果，包含距离、蛇身代价和父节点信息
 */
static BFSOut bfs_grid(const State &s, int sy, int sx)
{
    BFSOut out;

    // 初始化：所有距离设为无穷大，蛇身代价为0，父节点为-1
    for (int y = 0; y < H; y++)
        for (int x = 0; x < W; x++)
        {
            out.dist[y][x] = 1e9;     // 无穷大距离
            out.snake_cost[y][x] = 0; // 初始蛇身代价为0
            out.parent[y][x] = -1;    // 无父节点
        }

    // 使用优先队列进行带权重的搜索
    // 元组格式：(总代价, 蛇身代价, y坐标, x坐标)
    priority_queue<tuple<int, int, int, int>, vector<tuple<int, int, int, int>>, greater<tuple<int, int, int, int>>> pq;

    // 设置起始位置
    out.dist[sy][sx] = 0;
    out.snake_cost[sy][sx] = 0;
    pq.emplace(0, 0, sy, sx); // (总代价=0, 蛇身代价=0, 起始坐标)

    // BFS主循环：逐步扩展搜索范围
    while (!pq.empty())
    {
        auto current = pq.top(); // 取出代价最低的节点
        pq.pop();
        int total_cost = get<0>(current);
        int snake_steps = get<1>(current);
        int y = get<2>(current);
        int x = get<3>(current);

        // 如果已经找到更好的路径，跳过当前节点
        if (total_cost > out.dist[y][x] + out.snake_cost[y][x] * SNAKE_COST_WEIGHT)
            continue;

        // 尝试向四个方向扩展
        for (int k = 0; k < 4; k++)
        {
            int ny = y + DY[k], nx = x + DX[k]; // 计算新位置

            if (!in_bounds(ny, nx))
                continue; // 超出边界，跳过

            if (mp[ny][nx] == -5 && !s.self().has_key)
                continue; // 无法开启宝箱，跳过

            int new_dist = out.dist[y][x] + 1;         // 新的实际距离
            int new_snake_cost = out.snake_cost[y][x]; // 继承蛇身代价

            // === 蛇身穿越代价计算 ===
            // 如果新位置是其他蛇身或安全区外，需要额外代价
            if (go_but_need_shield(ny, nx))
            {
                // 情况1：当前已有护盾激活 - 低代价穿过
                if (s.snakes[s.self_idx].shield_time > 0)
                {
                    new_snake_cost += SNAKE_COST_WITH_SHIELD; // 有护盾时低代价
                }
                // 情况2：无护盾但可以开启 - 中等代价（需要先开盾）
                else if (can_open_shield())
                {
                    new_snake_cost += SNAKE_COST_NO_SHIELD; // 无护盾时高代价
                    new_dist += 1;                          // 开启护盾需要额外一回合
                }
                // 情况3：无护盾且无法开启 - 极高代价（强烈不推荐）
                else
                {
                    new_snake_cost += 10000; // 极高代价，几乎禁止
                }
            }

            // === 陷阱惩罚计算 ===
            // 对陷阱施加额外代价，软性避开但不完全禁止
            int extra_cost = 0;
            if (is_trap(ny, nx))
            {
                extra_cost += TRAP_STEP_COST * 2; // 陷阱额外惩罚
            }

            // 计算总代价：距离 + 蛇身代价*权重 + 陷阱惩罚
            int new_total_cost = new_dist + new_snake_cost * SNAKE_COST_WEIGHT + extra_cost;

            // 如果找到更优路径，更新并加入队列
            if (new_total_cost < out.dist[ny][nx] + out.snake_cost[ny][nx] * SNAKE_COST_WEIGHT)
            {
                out.dist[ny][nx] = new_dist;
                out.snake_cost[ny][nx] = new_snake_cost;
                out.parent[ny][nx] = (k + 2) % 4; // 存储反向方向（用于路径回溯）
                pq.emplace(new_total_cost, new_snake_cost, ny, nx);
            }
        }
    }
    return out;
}

// ==================== 决策算法辅助函数 ====================

/**
 * 计算敌人到指定点的最小曼哈顿距离
 * @param s 游戏状态
 * @param y 目标y坐标
 * @param x 目标x坐标
 * @return 最近敌人的曼哈顿距离
 */
static int min_opponent_distance(const State &s, int y, int x)
{
    int best = 1000000000;
    for (const auto &sn : s.snakes)
    {
        if (sn.id == MYID)
            continue;
        const auto &h = sn.head();
        if (h.y < 0 || h.y >= H || h.x < 0 || h.x >= W)
            continue;
        int md = std::abs(y - h.y) + std::abs(x - h.x);
        if (md < best)
            best = md;
    }
    return best;
}

/**
 * 检查指定位置是否在生命周期内可达（安全行径）
 * @param G BFS结果
 * @param s 游戏状态
 * @param y 目标y坐标
 * @param x 目标x坐标
 * @param life 生命周期限制(-1表示无限制)
 * @return tuple<是否可达, 距离, 蛇身代价>
 */
static tuple<bool, int, int> is_reachable(const BFSOut &G, const State &s, int y, int x, int life)
{
    int d = (in_bounds(y, x) ? G.dist[y][x] : (int)1e9);
    int snake_steps = (in_bounds(y, x) ? G.snake_cost[y][x] : (int)1e9);
    if (d >= (int)1e9)
        return make_tuple(false, d, snake_steps);
    if (life != -1 && d > life)
        return make_tuple(false, d, snake_steps);
    return make_tuple(true, d, snake_steps);
}

/**
 * 计算指定位置的局部自由度（四邻中可继续行走的数量）
 * @param s 游戏状态
 * @param y 目标y坐标
 * @param x 目标x坐标
 * @param has_key 是否持有钥匙
 * @return 可移动方向的数量
 */
static int calculate_local_degree(const State &s, int y, int x, bool has_key)
{
    int deg = 0;
    for (int t = 0; t < 4; ++t)
    {
        int py = y + DY[t], px = x + DX[t];
        if (!in_bounds(py, px))
            continue;
        if (!in_safe_zone(s.cur, py, px))
            continue;
        if (unsafe_to_go(py, px, has_key))
            continue;
        ++deg;
    }
    return deg;
}

/**
 * 求生优先策略函数：当没有好的目标时使用的兜底策略
 * @param s 游戏状态
 * @param sy 蛇头y坐标
 * @param sx 蛇头x坐标
 * @param log_ss 日志字符串流引用
 * @return 动作代码
 */
static int survival_strategy(const State &s, int sy, int sx, stringstream &log_ss)
{
    const auto &me = s.self();

    log_ss << "SURVIVAL_MODE:|";

    // === 策略1：寻找可达性最好的安全移动方向 ===
    int bestDir = -1;   // 最佳移动方向（-1表示未找到）
    int bestReach = -1; // 最佳方向的可达性得分

    log_ss << "SAFE_MOVE_ANALYSIS:|";
    // 遍历四个可能的移动方向
    for (int k = 0; k < 4; ++k)
    {
        int ny = sy + DY[k], nx = sx + DX[k]; // 计算下一步位置

        string direction_name;
        switch (k)
        {
        case 0:
            direction_name = "LEFT";
            break;
        case 1:
            direction_name = "UP";
            break;
        case 2:
            direction_name = "RIGHT";
            break;
        case 3:
            direction_name = "DOWN";
            break;
        }

        log_ss << direction_name << "@(" << ny << "," << nx << ")";

        // === 防止180度掉头检查 ===
        int opposite_dir = (me.dir + 2) % 4; // 计算上回合移动方向的相反方向
        if (k == opposite_dir)
        {
            log_ss << ":REVERSE_BLOCKED|";
            continue;
        }
        // 计算该位置的"可达性"：统计从该位置能继续移动的方向数量
        int deg = 0; // 可达方向计数器
        // 检查该位置是否安全可移动：
        // - 必须在地图边界内
        // - 必须在安全区域内
        // - 不能是阻挡位置（墙、陷阱、宝箱等）
        // - 不能是危险位置（敌蛇头附近）
        // - 不能是敌蛇身体
        // - 优先避免陷阱（在安全移动分析中）
        if (!in_bounds(ny, nx))
        {
            log_ss << ":OUT_OF_BOUNDS|";
            continue;
        }
        if (s.self().shield_time == 0)
        {
            if (!in_safe_zone(s.cur, ny, nx))
            {
                log_ss << ":UNSAFE|";
                continue;
            }
            if (unsafe_to_go(ny, nx, me.has_key))
            {
                log_ss << ":BLOCKED|";
                continue;
            }
            if (is_danger_zone(ny, nx, s))
            {
                deg = -1;
            }
            if (go_but_need_shield(ny, nx))
            {
                log_ss << ":SNAKE_BODY|";
                continue;
            }
        }
        // 在安全移动分析中避免陷阱 - 只有在所有其他选项都不安全时才会考虑陷阱
        if (is_trap(ny, nx))
        {
            deg = -2;
        }

        for (int t = 0; t < 4; ++t)
        {
            int py = ny + DY[t], px = nx + DX[t]; // 从候选位置继续移动的位置
            // 检查是否为安全可移动的位置
            if (in_bounds(py, px) && in_safe_zone(s.cur, py, px) && !unsafe_to_go(py, px, me.has_key) && !go_but_need_shield(py, px))
                ++deg;
        }

        log_ss << ":SAFE,r:" << deg << "|";

        // 选择可达性最好的方向（避免走入死胡同）
        if (deg > bestReach)
        {
            bestReach = deg;
            bestDir = k;
        }
    }

    // 如果找到了安全的移动方向，立即执行
    if (bestDir != -1)
    {
        string best_direction;
        switch (bestDir)
        {
        case 0:
            best_direction = "LEFT";
            break;
        case 1:
            best_direction = "UP";
            break;
        case 2:
            best_direction = "RIGHT";
            break;
        case 3:
            best_direction = "DOWN";
            break;
        }
        log_ss << "SAFE_MOVE_CHOSEN:" << best_direction << ",a:" << ACT[bestDir]
               << ",r:" << bestReach << "|";
        return ACT[bestDir];
    }
    // 尝试经过陷阱（但仍避免蛇身和其他障碍）
    log_ss << "TRAP_DESPERATE_MOVE_ANALYSIS:|";
    for (int k = 0; k < 4; ++k)
    {
        int ny = sy + DY[k], nx = sx + DX[k];

        string direction_name;
        switch (k)
        {
        case 0:
            direction_name = "LEFT";
            break;
        case 1:
            direction_name = "UP";
            break;
        case 2:
            direction_name = "RIGHT";
            break;
        case 3:
            direction_name = "DOWN";
            break;
        }

        log_ss << direction_name << "@(" << ny << "," << nx << ")";

        // === 防止180度掉头检查 ===
        int opposite_dir = (me.dir + 2) % 4;
        if (k == opposite_dir)
        {
            log_ss << ":REVERSE_BLOCKED|";
            continue;
        }

        // 检查基本边界、阻挡和蛇身，但允许陷阱通过
        if (in_bounds(ny, nx) && in_safe_zone(s.cur, ny, nx) && !unsafe_to_go(ny, nx, me.has_key) &&
            (!go_but_need_shield(ny, nx) || me.shield_time > 0))
        {
            // 即使是陷阱也允许通过，作为最后的求生手段
            log_ss << ":TRAP_DESPERATE_CHOSEN";
            if (is_trap(ny, nx))
            {
                log_ss << "(TRAP_ACCEPTED)";
            }
            log_ss << "|";
            return ACT[k];
        }
        else
        {
            log_ss << ":NOT_VIABLE|";
        }
    }
    // === 策略2：如果无安全移动，尝试开启护盾 ===
    // 条件：当前未开启护盾且满足开盾条件（分数足够且冷却完毕）
    if (me.shield_time == 0 && can_open_shield())
    {
        log_ss << "SHIELD_ACTIVATION:|";
        return 4; // 动作4 = 开启护盾
    }

    // === 策略4：绝望的护盾尝试 ===
    // 如果连基本移动都不可能，强制尝试开启护盾（即使在冷却中）
    log_ss << "FORCED_SHIELD:|";
    // 原实现返回 1(上) 与日志含义(护盾) 不一致，修正为返回护盾动作 4
    return 4;
}

// ==================== 决策算法 ====================

/**
 * 选择结构：表示AI的决策结果
 */
struct Choice
{
    int action; // 选择的动作代码（0-4）
};

/**
 * 主决策函数：基于当前游戏状态做出最优移动决策
 *
 * 算法核心流程：
 * 1. 构建地图状态和威胁检测
 * 2. 处理紧急情况（安全区外、头撞头等）
 * 3. 执行全图BFS寻找最优路径
 * 4. 评估所有可达目标的价值
 * 5. 使用前瞻算法优化决策
 * 6. 应用安全性检查和护盾策略
 * 7. 返回最优动作选择
 *
 * 特色功能：
 * - 智能护盾使用：根据情况决定何时开启护盾
 * - 竞争感知：考虑对手到达目标的时间
 * - 缩圈预警：提前规避安全区收缩风险
 * - 陷阱权衡：在绝望时允许通过陷阱
 * - 头撞头检测：避免与敌蛇正面冲突
 *
 * @param s 当前游戏状态
 * @return 包含动作代码的选择结构
 */
static Choice decide(const State &s)
{
    const auto &me = s.self();
    build_map(s);                                 // 1) 构建地图
    const int sy = me.head().y, sx = me.head().x; // 当前蛇头坐标

    stringstream log_ss; // 使用 stringstream 高效构建日志
    log_ss << "TICK:" << s.current_ticks << "|"
           << "POSITION:" << sy << "," << sx << "|"
           << "SCORE:" << me.score << "|"
           << "LENGTH:" << me.length << "|"
           << "SHIELD_COOLDOWN:" << me.shield_cd << "|"
           << "SHIELD_TIME:" << me.shield_time << "|";

    // === 先处理"是否必须立刻开盾"的紧急场景 ===
    // 1) 头部当前已在安全区外：首要任务是回到安全区
    if (!in_bounds(sy, sx) || !in_safe_zone(s.cur, sy, sx))
    {
        log_ss << "OUTSIDE_SAFE_ZONE:EMERGENCY_RETURN|";

        // 如果当前没有护盾且可以开盾，立即开盾保命
        if (me.shield_time == 0 && can_open_shield())
        {
            log_ss << "FORCE_SHIELD:OUT_OF_SAFE_ZONE|";
            str_info += log_ss.str();
            return {4};
        }

        // 如果有护盾，寻找最近的安全区入口，忽略敌人蛇身
        log_ss << "SEEKING_SAFE_ZONE_ENTRY|";

        // 计算到安全区边界的最短路径
        int best_dir = -1;
        int min_distance = INT_MAX;

        // 遍历四个方向，寻找能最快到达安全区的路径
        for (int k = 0; k < 4; k++)
        {
            int ny = sy + DY[k], nx = sx + DX[k];

            // 检查边界
            if (!in_bounds(ny, nx))
                continue;

            // 计算到最近安全区边界的距离
            int dist_to_safe = INT_MAX;

            // 如果这个位置已经在安全区内，距离为0
            if (in_safe_zone(s.cur, ny, nx))
            {
                dist_to_safe = 0;
            }
            else
            {
                // 计算到安全区边界的曼哈顿距离
                int dx_to_safe = 0, dy_to_safe = 0;

                if (nx < s.cur.x_min)
                    dx_to_safe = s.cur.x_min - nx;
                else if (nx > s.cur.x_max)
                    dx_to_safe = nx - s.cur.x_max;

                if (ny < s.cur.y_min)
                    dy_to_safe = s.cur.y_min - ny;
                else if (ny > s.cur.y_max)
                    dy_to_safe = ny - s.cur.y_max;

                dist_to_safe = dx_to_safe + dy_to_safe;
            }

            log_ss << "DIR" << k << "@(" << ny << "," << nx << ")dist:" << dist_to_safe << "|";

            // 更新最佳方向
            if (dist_to_safe < min_distance)
            {
                min_distance = dist_to_safe;
                best_dir = k;
            }
        }

        // 如果找到了通向安全区的方向
        if (best_dir != -1)
        {
            string direction_name;
            switch (best_dir)
            {
            case 0:
                direction_name = "LEFT";
                break;
            case 1:
                direction_name = "UP";
                break;
            case 2:
                direction_name = "RIGHT";
                break;
            case 3:
                direction_name = "DOWN";
                break;
            }

            log_ss << "RETURN_TO_SAFE:" << direction_name << ",dist:" << min_distance << "|";
            str_info += log_ss.str();
            return {ACT[best_dir]};
        }
        else
        {
            // 如果没有找到安全路径：能开盾就开盾；否则走兜底策略（别空放护盾指令）
            if (can_open_shield())
            {
                log_ss << "NO_SAFE_PATH:EMERGENCY_SHIELD|";
                str_info += log_ss.str();
                return {4};
            }
            else
            {
                // 无法开盾时使用兜底策略：尝试任何可能的移动方向避免死亡
                log_ss << "NO_SAFE_PATH:TRYING_ANY_DIRECTION|";

                // 尝试四个方向寻找任何可能的移动
                for (int k = 0; k < 4; k++)
                {
                    int ny = sy + DY[k], nx = sx + DX[k];

                    // 防止掉头
                    int opposite_dir = (me.dir + 2) % 4;
                    if (k == opposite_dir)
                        continue;

                    // 基本的边界和安全区检查
                    if (in_bounds(ny, nx) && in_safe_zone(s.cur, ny, nx))
                    {
                        str_info += log_ss.str();
                        return {ACT[k]};
                    }
                }

                // 如果连基本移动都不可能，只能默认向左移动
                log_ss << "ULTIMATE_FALLBACK:LEFT|";
                str_info += log_ss.str();
                return {0};
            }
        }
    }

    // 2) 预计算敌蛇头部可能的下一步位置的权值（用于头撞头风险检测）
    int opp_next[H][W];
    memset(opp_next, 0, sizeof(opp_next));
    for (int snake_idx = 0; snake_idx < (int)s.snakes.size(); snake_idx++)
    {
        const auto &sn = s.snakes[snake_idx];
        if (sn.id == MYID)
            continue; // 跳过自己
        auto head = sn.head();
        // 预测敌蛇头部四个方向的可能位置
        for (int k = 0; k < 4; k++)
        {
            int ny = head.y + DY[k], nx = head.x + DX[k];
            if (in_bounds(ny, nx) && in_safe_zone(s.cur, ny, nx))
            {
                opp_next[ny][nx] = is_valuable(snake_idx, ny, nx); // 使用索引 snake_idx
            }
        }
    }

    // 3) 以当前蛇头为起点做一次全图 BFS
    BFSOut G = bfs_grid(s, sy, sx);
    struct Target
    {
        int y, x;
        double score;
        int dist;
        int snake_cost;
    };
    vector<Target> cand;
    cand.reserve(64);

    log_ss << "ITEMS_ANALYSIS:|";

    // 4) 枚举地图上可作为目标的物品，构建候选列表
    for (const auto &it : s.items)
    {
        auto reachable_result = is_reachable(G, s, it.pos.y, it.pos.x, it.lifetime);
        bool ok = get<0>(reachable_result);
        int d = get<1>(reachable_result);
        int snake_steps = get<2>(reachable_result);
        if (!ok)
            continue;
        if (it.type == -2)
            continue;
        if (it.type == -5)
            continue;
        if (it.type == -3 && me.has_key)
            continue;

        // 如果物品周围敌蛇多则跑
        int mun = 0;
        for (int i = 0; i < 4; i++)
        {
            int yy = it.pos.y + DY[i], xx = it.pos.x + DX[i];
            if (!in_bounds(yy, xx) || !in_safe_zone(s.cur, yy, xx) || !unsafe_to_go(yy, xx, me.has_key))
                mun++;
        }
        if (mun >= 3)
        {
            continue;
        }
        double v = it.value;

        // 安全性惩罚：穿过蛇身的路径降低评分
        double safety_penalty = DISTANCE_OFFSET + snake_steps * SNAKE_SAFETY_PENALTY_RATE; // 每个蛇身格子降低指定比例的效率

        // 竞争检测：如果敌人能同样快或更快到达，降低此目标的吸引力
        int d_opp = min_opponent_distance(s, it.pos.y, it.pos.x);
        double contest_factor = (d_opp <= d ? CONTEST_PENALTY : 1.0);

        // 下一安全区风险检测：如果到达时已经缩圈且目标不在下一安全区内，降低吸引力
        double zone_factor = 1.0;
        if (s.next_tick != -1 && (s.current_ticks + d) >= s.next_tick)
        {
            if (!in_safe_zone(s.next, it.pos.y, it.pos.x))
                zone_factor = NEXTZONE_RISK_PENALTY;
        }

        double degree_factor = 1.0 + DEGREE_BONUS * calculate_local_degree(s, it.pos.y, it.pos.x, s.self().has_key);
        double sc = (v * (it.lifetime == -1 ? 1.0 : pow(LIFETIME_SOFT_DECAY, d))) / ((d + DISTANCE_OFFSET) * safety_penalty) * contest_factor * zone_factor * degree_factor;
        cand.push_back({it.pos.y, it.pos.x, sc, d, snake_steps});

        // 详细日志：记录候选目标
        string item_type;
        switch (it.type)
        {
        case -1:
            item_type = "GROWTH_FOOD";
            break;
        case -3:
            item_type = "KEY";
            break;
        case -5:
            item_type = "CHEST";
            break;
        default:
            item_type = "NORMAL_FOOD(" + to_string(it.type) + ")";
            break;
        }
        log_ss << item_type << "@(" << it.pos.y << "," << it.pos.x
               << ")d:" << d << ",s:" << snake_steps
               << ",sc:" << (int)(sc * SCORE_DISPLAY_MULTIPLIER) << "|";
    }

    if (me.has_key)
    {
        log_ss << "CHEST_ANALYSIS:|";
        for (const auto &c : s.chests)
        {
            auto reachable_result2 = is_reachable(G, s, c.pos.y, c.pos.x, -1);
            bool ok = get<0>(reachable_result2);
            int d = get<1>(reachable_result2);
            int snake_steps = get<2>(reachable_result2);
            if (!ok)
                continue;
            double safety_penalty = DISTANCE_OFFSET + snake_steps * SNAKE_SAFETY_PENALTY_RATE;

            // 竞争检测：如果敌人能同样快或更快到达，降低此目标的吸引力
            int d_opp = min_opponent_distance(s, c.pos.y, c.pos.x);
            double contest_factor = (d_opp <= d ? CONTEST_PENALTY + 0.2 : 1.0);

            // 下一安全区风险检测：如果到达时已经缩圈且目标不在下一安全区内，降低吸引力
            double zone_factor = 1.0;
            if (s.next_tick != -1 && (s.current_ticks + d) >= s.next_tick)
            {
                if (!in_safe_zone(s.next, c.pos.y, c.pos.x))
                    zone_factor = NEXTZONE_RISK_PENALTY;
            }

            double degree_factor = 1.0 + DEGREE_BONUS * calculate_local_degree(s, c.pos.y, c.pos.x, s.self().has_key);
            double sc = (c.score > 0 ? c.score * CHEST_SCORE_MULTIPLIER : DEFAULT_CHEST_SCORE) / ((d + DISTANCE_OFFSET) * safety_penalty) * contest_factor * zone_factor * degree_factor;
            cand.push_back({c.pos.y, c.pos.x, sc, d, snake_steps});

            log_ss << "CHEST@(" << c.pos.y << "," << c.pos.x
                   << ")d:" << d << ",s:" << snake_steps
                   << ",sc:" << (int)(sc * SCORE_DISPLAY_MULTIPLIER) << "|";
        }
    }

    log_ss << "CANDIDATES_COUNT:" << cand.size() << "|";

    // 5) 如果没有任何候选目标：执行“求生优先”兜底策略
    if (cand.empty())
    {
        int choice = survival_strategy(s, sy, sx, log_ss);
        str_info += log_ss.str();
        return {choice};
    }
    sort(cand.begin(), cand.end(), [](const Target &a, const Target &b)
         {
        if (fabs(a.score - b.score) > 1e-9) return a.score > b.score;
        return a.dist < b.dist; });
    int ans = -1;
    for (const auto &target : cand)
    {
        // 6) 选择评分最高的目标

        log_ss << "TARGET_SELECTED:(" << target.y << "," << target.x
               << ")sc:" << (int)(target.score * SCORE_DISPLAY_MULTIPLIER) << ",d:" << target.dist
               << ",s:" << target.snake_cost << "|";

        // 7) 从目标位置回溯到蛇头，找到第一步应该走的方向
        int ty = target.y, tx = target.x; // 目标位置坐标

        // 检查目标位置是否可达（是否有父节点）
        if (G.parent[ty][tx] == -1)
        {
            continue;
        }

        // === 路径回溯算法：从目标回溯到蛇头的下一步位置 ===
        int cy = ty, cx = tx; // 当前回溯位置，从目标开始
        log_ss << "PATH_BACKTRACK:|";

        // 沿着父节点链向蛇头方向回溯
        while (!(cy == sy && cx == sx)) // 直到回溯到蛇头位置
        {
            int back = G.parent[cy][cx];                // 获取当前位置的父节点方向
            int py = cy + DY[back], px = cx + DX[back]; // 计算父节点位置

            log_ss << "(" << cy << "," << cx << ")<-(" << py << "," << px << ")|";

            // 如果父节点就是蛇头，说明找到了下一步要去的位置
            if (py == sy && px == sx)
                break;

            // 继续向父节点回溯
            cy = py;
            cx = px;
        }

        // === 将回溯得到的位置转换为移动方向 ===
        int dir = -1; // 移动方向（-1表示未找到）

        // 遍历四个方向，找到从蛇头到下一步位置的方向
        for (int k = 0; k < 4; ++k)
            if (sy + DY[k] == cy && sx + DX[k] == cx)
            {
                dir = k; // 找到对应的方向索引
                break;
            }

        // 检查是否成功找到移动方向
        if (dir == -1)
        {
            continue;
        }

        // === 防止180度掉头检查 ===
        // 蛇不能直接向相反方向移动（掉头）
        int opposite_dir = (me.dir + 2) % 4; // 计算上回合移动方向的相反方向
        if (dir == opposite_dir)
        {
            continue;
        }

        // === 主动护盾策略：头撞头风险和安全区检测 ===
        // 1) 头撞头风险：若我们计划前进到的格子，被对手下回合也有可能占据
        // 且当前没有护盾，则优先尝试换一个安全方向；如无路可换且能开盾，则开盾
        if (dir != -1)
        {
            int ty = sy + DY[dir], tx = sx + DX[dir];
            if (opp_next[ty][tx] >= 2 && me.shield_time == 0)
            {
                log_ss << "HEAD_TO_HEAD_RISK_DETECTED|";
                // 尝试在四邻中找一个既安全又不被对手争抢的方向
                bool found_alternative = false;
                for (int k = 0; k < 4; k++)
                {
                    int ny = sy + DY[k], nx = sx + DX[k];
                    if (!in_bounds(ny, nx) || !in_safe_zone(s.cur, ny, nx))
                        continue;
                    if (unsafe_to_go(ny, nx, me.has_key))
                        continue;
                    if (opp_next[ny][nx] <= 1)
                    {
                        dir = k;
                        // 同步第一步坐标，避免日志与动作不一致
                        cy = sy + DY[dir];
                        cx = sx + DX[dir];
                        found_alternative = true;
                        log_ss << "ALTERNATIVE_DIRECTION_FOUND|";
                        break;
                    }
                }
                // 如果仍然没有可换的方向，尝试开盾保命
                if (!found_alternative)
                {
                    continue;
                }
            }
        }

        // 2) 如果下一步将离开安全区，尽量换一个仍在安全区内的方向；实在不行且可开盾，则开盾顶住
        if (dir != -1)
        {
            int ty = sy + DY[dir], tx = sx + DX[dir];
            if ((!in_bounds(ty, tx) || !in_safe_zone(s.cur, ty, tx)) && me.shield_time == 0)
            {
                log_ss << "LEAVING_SAFE_ZONE_RISK|";
                bool changed = false;
                for (int k = 0; k < 4; ++k)
                {
                    int ny = sy + DY[k], nx = sx + DX[k];
                    if (!in_bounds(ny, nx) || !in_safe_zone(s.cur, ny, nx))
                        continue;
                    if (unsafe_to_go(ny, nx, me.has_key))
                        continue;
                    if (in_bounds(ny, nx) && in_safe_zone(s.cur, ny, nx))
                    {
                        dir = k;
                        cy = sy + DY[dir];
                        cx = sx + DX[dir];
                        changed = true;
                        log_ss << "SAFE_ZONE_ALTERNATIVE_FOUND|";
                        break;
                    }
                }
                if (!changed && can_open_shield())
                {
                    continue;
                }
            }
        }

        string next_direction;
        switch (dir)
        {
        case 0:
            next_direction = "LEFT";
            break;
        case 1:
            next_direction = "UP";
            break;
        case 2:
            next_direction = "RIGHT";
            break;
        case 3:
            next_direction = "DOWN";
            break;
        }

        log_ss << "NEXT_STEP:" << next_direction << ",a:" << ACT[dir]
               << "@(" << cy << "," << cx << ")|";

        // 9) 蛇身穿越检测：智能护盾使用策略
        if (unsafe_to_go(cy, cx, me.has_key)) // 如果下一步位置有敌方蛇身
        {
            log_ss << "SNAKE_BODY_DETECTED:|";
            // === 情况1：当前已有护盾激活 ===
            if (me.shield_time > 0)
            {
                // 直接移动，利用现有护盾穿过蛇身
                log_ss << "SHIELD_PASS:" << next_direction << ",a:" << ACT[dir] << "|";
                str_info += log_ss.str();
                return {ACT[dir]};
            }
            // === 情况2：没有护盾但可以激活 ===
            continue;
        }
        // 10) 正常移动：所有检查通过，执行计划的移动
        log_ss << "NORMAL_MOVE:" << next_direction << ",a:" << ACT[dir] << "|";
        str_info += log_ss.str();
        return {ACT[dir]}; // 返回对应的移动动作
    }
    int final_choice = survival_strategy(s, sy, sx, log_ss);
    return {final_choice};
}

// ==================== 主程序入口 ====================

/**
 * 主函数
 *
 * 程序流程：
 * 1. 读取游戏状态
 * 2. 执行决策算法
 * 3. 输出动作选择
 *
 * 这是一个高效的贪吃蛇AI，专注于：
 * - 快速BFS路径规划
 * - 智能的目标评估
 * - 安全性优先的移动策略
 * - 在0.7秒时间限制内完成所有计算
 */
int main()
{
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    read_state(global_state);
    auto choice = decide(global_state);
    cout << choice.action << "\n";
    cout << str_info << "\n";
    return 0;
}
