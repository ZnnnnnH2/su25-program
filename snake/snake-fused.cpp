#include <bits/stdc++.h>
using namespace std;

/**
 * 多人贪吃蛇竞技AI - 战术型实现
 * 1. 多阶段BFS路径规划算法
 * 2. 智能竞争对手分析系统
 * 3. 动态食物价值评估
 * 4. 安全区域收缩适配机制
 * 5. 实时碰撞预测与规避
 *
 * 重要规则：与自己的蛇不会发生碰撞
 */

// ==================== 游戏常量定义 ====================
// ==================== 游戏地图和玩家常量 ====================
static constexpr int W = 40;            // 地图宽度 (x坐标范围: 0-39)
static constexpr int H = 30;            // 地图高度 (y坐标范围: 0-29)
static constexpr int MYID = 2024201540; // 学号标识符 - 必须设置为你的学号

// ==================== 护盾系统和移动代价常量 ====================
// 用于精确控制AI在有/无护盾状态下的行为决策
static constexpr int SNAKE_COST_WEIGHT = 10;     // 蛇身移动基础代价权重
static constexpr int SNAKE_COST_NO_SHIELD = 100; // 无护盾时强制避开蛇身的高代价
static constexpr int SNAKE_COST_WITH_SHIELD = 0; // 有护盾时可穿越蛇身的零代价
static constexpr int SNAKE_COST_OPEN_SHIELD = 2; // 主动开启护盾的时间代价
static constexpr int SHIELD_COST_THRESHOLD = 20; // 开启护盾所需的最低分数阈值
static constexpr int TRAP_STEP_COST = 30;        // 踩踏陷阱的路径规划惩罚代价

// ==================== 食物和物品价值常量 ====================
/**
 * 游戏阶段和食物生成率常量
 * 基于官方游戏规则中的食物生成权重分布
 */
// 游戏阶段时间节点常量
static constexpr int EARLY_STAGE_END = 80;      // 早期阶段结束时间 (1-80回合)
static constexpr int MID_STAGE_END = 200;       // 中期阶段结束时间 (81-200回合)
static constexpr int LATE_STAGE_END = 256;      // 后期阶段结束时间 (201-256回合)
static constexpr int LATE_GAME_THRESHOLD = 160; // 后期游戏阈值，用于策略调整

// 成长食物基础价值评估
static constexpr int GROWTH_FOOD_VALUE = 8; // 成长食物的基础价值评估

// 蛇长度相关常量
static constexpr int SMALL_SNAKE_THRESHOLD = 8;          // 小蛇长度阈值
static constexpr int MEDIUM_SNAKE_THRESHOLD = 12;        // 中型蛇长度阈值
static constexpr int MEDIUM_LARGE_SNAKE_THRESHOLD = 10;  // 中等大小蛇长度阈值
static constexpr int LARGE_SNAKE_THRESHOLD = 15;         // 大蛇长度阈值
static constexpr int VERY_LARGE_SNAKE_THRESHOLD = 20;    // 超大蛇长度阈值
static constexpr double SMALL_SNAKE_GROWTH_BONUS = 1.3;  // 小蛇成长奖励倍数(30%)
static constexpr double MEDIUM_SNAKE_GROWTH_BONUS = 1.1; // 中型蛇成长奖励倍数(10%)

static constexpr int TRAP_PENALTY = -10;         // 陷阱扣分惩罚（负值，符合游戏规则-10分）
static constexpr int KEY_VALUE = 75;             // 钥匙的基础价值评估
static constexpr int CHEST_VALUE = 100;          // 宝箱的基础价值评估
static constexpr int NORMAL_FOOD_MULTIPLIER = 4; // 普通食物价值倍数

// ==================== 评分和权重常量 ====================
/**
 * 目标评分系统的核心参数
 * 用于平衡距离、价值、安全性、竞争优势等多个因素
 */
static constexpr double SNAKE_SAFETY_PENALTY_RATE = 0.5; // 穿越蛇身的安全惩罚率
static constexpr double CHEST_SCORE_MULTIPLIER = 2.0;    // 宝箱评分加成倍数
static constexpr double DEFAULT_CHEST_SCORE = 100.0;     // 宝箱默认分数
static constexpr double DISTANCE_OFFSET = 1.0;           // 距离计算偏移量，防止除零错误
static constexpr int SCORE_DISPLAY_MULTIPLIER = 100;     // 调试信息显示时的分数放大倍数
static constexpr double LIFETIME_SOFT_DECAY = 0.90;      // 食物生命值衰减系数
static constexpr double CONTEST_PENALTY = 0.6;           // 竞争目标惩罚系数（敌人同样或更快到达）
static constexpr double NEXTZONE_RISK_PENALTY = 0.1;     // 下个安全区风险折扣
static constexpr double DEGREE_BONUS = 0.06;             // 局部自由度奖励系数

// ==================== 前瞻算法常量 ====================
static constexpr int LOOKAHEAD_DEPTH = 1; // 前瞻深度：1=启用一步前瞻，0=关闭

// ==================== 风险评估常量 ====================
/**
 * 危险检测和回避系统的阈值设定
 * 用于识别并规避各种游戏风险
 */
static constexpr int SAFE_ZONE_BOUNDARY_THRESHOLD = 2;   // 安全区边界危险检测半径
static constexpr int SAFE_ZONE_SHRINK_THRESHOLD = 5;     // 安全区收缩预警阈值（回合数）
static constexpr int ENEMY_BODY_PROXIMITY_THRESHOLD = 1; // 敌蛇身体危险邻近阈值
static constexpr int TRAP_PROXIMITY_THRESHOLD = 1;       // 陷阱危险邻近阈值
static constexpr int NEAR_ENEMY_ADJ_PENALTY = 1;         // 邻近敌蛇身体的额外移动代价

// ===== 新增：三个改进所需的可调参数 =====
static constexpr int FUTURE_SAFE_FALLBACK_ALLOW_UTURN = 0; // 避免立即U型转弯（如果可能）
static constexpr int TTL_DECAY_GRACE_TICKS = 1;            // TTL软衰减的宽限期
static constexpr double TTL_DECAY_TAU = 4.0;               // TTL衰减时间常数，值越大衰减越慢

// ==================== 路径选择偏好常量 ====================
/**
 * 路径质量评估系统
 * 用于在多条路径中选择最安全、最有利的路线
 */
static constexpr double OPEN_AREA_BONUS = 1;    // 开阔区域移动奖励倍数
static constexpr int DEAD_END_PENALTY = 200;    // 死路（逃生路线不足）的严重惩罚
static constexpr int NARROW_PATH_PENALTY = 100; // 狭窄路径的惩罚分数

// ==================== 食物密度奖励常量 ====================
/**
 * 食物密集区域检测系统
 * 优先选择食物密集的区域以提高收集效率
 */
static constexpr int FOOD_DENSITY_RADIUS = 4;          // 食物密度检测半径
static constexpr double FOOD_DENSITY_BONUS_BASE = 0.3; // 食物密度基础奖励系数
static constexpr double FOOD_DENSITY_BONUS_MAX = 2;    // 食物密度最大奖励倍数
static constexpr int MIN_FOOD_COUNT_FOR_BONUS = 2;     // 获得密度奖励的最小食物数量
static constexpr int SAFE_ZONE_BOUNDARY_PENALTY = 50;  // 安全区边界位置惩罚
static constexpr int MIN_REWARD_THRESHOLD = 100;       // 最小奖励阈值（低于此值的路径将被重罚）
static constexpr int OPENNESS_RADIUS = 2;              // 开阔度检测半径
static constexpr int MIN_ESCAPE_ROUTES = 2;            // 最小逃生路线数量要求

// ==================== 碰撞预测常量 ====================
/**
 * 多步碰撞预测系统
 * 预测未来几步的潜在碰撞风险并提前规避
 */
static constexpr int COLLISION_PREDICTION_STEPS = 4;    // 碰撞预测步数
static constexpr int COLLISION_AVOIDANCE_RADIUS = 2;    // 碰撞规避检测半径
static constexpr double COLLISION_RISK_THRESHOLD = 0.7; // 碰撞风险阈值

// ==================== 数据结构定义 ====================
/**
 * 游戏核心数据结构
 * 设计原则：与游戏引擎API格式完全对齐，确保数据解析的准确性
 */

/**
 * 二维坐标点结构
 * 注意：y为行坐标，x为列坐标（符合矩阵索引习惯）
 */
struct Point
{
    int y, x; // y: 行坐标[0, H-1], x: 列坐标[0, W-1]

    // 添加相等运算符
    bool operator==(const Point &other) const
    {
        return y == other.y && x == other.x;
    }
};

/**
 * 游戏物品结构
 * value字段的语义定义：
 * - 正数(1-5): 普通食物的分数
 * - -1: 成长食物（使蛇身增长1段）
 * - -2: 陷阱（踩踏扣10分）
 * - -3: 钥匙（打开宝箱必需）
 * - -5: 宝箱（需要钥匙打开）
 */
struct Item
{
    Point pos;    // 物品在地图上的位置坐标
    int type;     // 物品类型标识符
    int value;    // 经过AI计算后的物品价值
    int lifetime; // 物品剩余存在时间（-1表示永久存在）
};

/**
 * 蛇的完整状态信息
 * 包含所有游戏逻辑需要的蛇的属性
 */
struct Snake
{
    int id, length, score, dir;                        // ID标识、身体长度、当前分数、移动方向
    int shield_cd;                                     // 护盾冷却时间（0表示可用）
    int shield_time;                                   // 护盾剩余持续时间
    bool has_key = false;                              // 是否持有钥匙（用于开启宝箱）
    vector<Point> body;                                // 蛇身体各段的坐标（索引0为蛇头）
    const Point &head() const { return body.front(); } // 便捷方法：获取蛇头位置
};

/**
 * 宝箱结构
 * 需要钥匙才能打开的高价值目标
 */
struct Chest
{
    Point pos; // 宝箱位置坐标
    int score; // 宝箱内的分数奖励
};

/**
 * 钥匙结构
 * 打开宝箱的必需道具
 */
struct Key
{
    Point pos;          // 钥匙位置坐标
    int holder_id;      // 当前持有者的蛇ID（-1表示未被拾取）
    int remaining_time; // 钥匙的剩余有效时间
};

/**
 * 安全区域边界定义
 * 用于表示当前、下一次和最终的安全区域范围
 */
struct Safe
{
    int x_min, y_min, x_max, y_max; // 安全区域的矩形边界坐标
};

/**
 * 游戏完整状态结构
 * 包含每回合从游戏引擎接收到的所有信息
 */
struct State
{
    int current_ticks;                 // 当前游戏回合数(1-256)
    int remaining_ticks;               // 剩余游戏回合数
    vector<Item> items;                // 地图上的所有物品（食物、陷阱、钥匙等）
    vector<Snake> snakes;              // 所有参与游戏的蛇
    vector<Chest> chests;              // 地图上的所有宝箱
    vector<Key> keys;                  // 地图上的所有钥匙
    vector<Point> traps;               // 地图上的所有陷阱位置（从items中提取）
    Safe cur, next, fin;               // 当前、下次收缩、最终安全区域
    int next_tick = -1, fin_tick = -1; // 安全区收缩的时间节点
    int self_idx = -1;                 // 自己的蛇在snakes数组中的索引

    // 便捷方法：获取自己的蛇对象引用
    const Snake &self() const { return snakes[self_idx]; }
};

/**
 * 路径规划结构
 * 用于存储AI计算出的最优目标序列
 */
struct Route
{
    vector<Point> seq; // 目标访问序列（包含食物、成长食物、钥匙、宝箱等所有有价值的目标）
    int totalVal = 0;  // 路径总预期价值
    int finishT = 0;   // 完成整条路径所需时间
};

/**
 * 时间线结构
 * 用于验证路径的可行性和时间约束
 */
struct Timeline
{
    vector<int> arrive, leave; // 到达和离开各个目标的时间点
    bool feasible = true;      // 路径是否在时间约束内可行
};

/**
 * 竞争分析结构
 * 分析与其他蛇争夺同一目标的优劣势
 */
struct CompetitionAnalysis
{
    Point target;     // 争夺目标的位置
    int my_dist;      // 我方到达目标的距离
    int enemy_dist;   // 敌方到达目标的距离
    int enemy_id;     // 竞争对手的蛇ID
    bool i_win_tie;   // 在距离相等情况下我方是否获胜
    double advantage; // 竞争优势评分（正数表示我方有利）
};

/**
 * 候选目标结构
 * 存储目标评估的详细信息
 */
struct Cand
{
    Point p;      // 目标位置坐标
    int val;      // 目标原始价值
    int d;        // 到达目标的距离
    double score; // 经过AI评估的综合评分
    int ttl;      // 目标剩余存在时间
};

// ==================== 核心游戏逻辑函数 ====================

/**
 * 平局胜负判定函数
 * 当多条蛇同时到达同一目标时，按照以下优先级判定获胜者：
 * 1. 距离最近者获胜
 * 2. 距离相等时，有护盾者优于无护盾者
 * 3. 护盾状态相同时，长度更长者获胜
 * 4. 长度相同时，分数更高者获胜
 * 5. 最后按ID大小决定（确保结果确定性）
 *
 * @param me 我方蛇状态
 * @param enemy 敌方蛇状态
 * @param target 争夺目标位置
 * @param my_dist 我方到目标距离
 * @param enemy_dist 敌方到目标距离
 * @return true表示我方获胜，false表示敌方获胜
 */
bool determine_tie_winner(const Snake &me, const Snake &enemy, const Point &target, int my_dist, int enemy_dist)
{
    // 距离优先：更近者获胜
    if (my_dist < enemy_dist)
        return true;
    if (my_dist > enemy_dist)
        return false;

    // 护盾优势：有护盾者在平局中优先
    if (me.shield_time > 0 && enemy.shield_time == 0)
        return true;
    if (me.shield_time == 0 && enemy.shield_time > 0)
        return false;
    return false;
}

/**
 * 护盾使用条件检查
 * 判断是否可以主动开启护盾
 *
 * @param me 当前蛇状态
 * @return true表示可以开启护盾
 */
inline bool can_open_shield(const Snake &me)
{
    return me.shield_cd == 0 && me.score >= SHIELD_COST_THRESHOLD;
}

string str_info;
State global_state; // 全局游戏状态

// ==================== 常量定义 ====================
static constexpr int INF_DIST = 1000000000; // BFS算法中的无穷大距离常量

// ==================== 辅助函数：游戏阶段和食物价值计算 ====================

/**
 * 计算成长食物的动态价值
 * 基于当前游戏阶段、蛇的状态等因素进行智能评估
 *
 * @param current_tick 当前游戏回合数
 * @param snake_length 当前蛇的长度（用于计算成长潜力）
 * @param snake_score 当前蛇的分数（用于战略价值评估）
 * @return 计算得出的成长食物价值
 */
inline int calculate_growth_food_value(int current_tick, int snake_length, int snake_score)
{
    // 根据游戏阶段确定基础价值
    int base_value = GROWTH_FOOD_VALUE;

    // 基于蛇长度的价值调整：短蛇更需要成长
    double length_multiplier = 1.0;
    if (snake_length < SMALL_SNAKE_THRESHOLD)
    {
        length_multiplier = SMALL_SNAKE_GROWTH_BONUS; // 小蛇获得成长奖励
    }
    else if (snake_length < MEDIUM_SNAKE_THRESHOLD)
    {
        length_multiplier = MEDIUM_SNAKE_GROWTH_BONUS; // 中型蛇获得成长奖励
    }
    else
    {
        length_multiplier = 0; // 大蛇不再获得成长奖励
    }

    return (int)(base_value * length_multiplier);
}

// ==================== 输入输出：每回合读取游戏状态 ====================

/**
 * 从标准输入读取游戏状态（API格式）
 * 优化了输入性能
 */
static void read_state(State &s)
{

    // 读取剩余回合数，如果读取失败则退出
    if (!(cin >> s.remaining_ticks))
    {
        cout << "0\n|ERR:FAILED_TO_READ_TICKS\n";
        exit(0);
    }
    s.current_ticks = LATE_STAGE_END - s.remaining_ticks;
    // ========== 读取物品信息 ==========
    int m;
    cin >> m;
    s.items.resize(m);
    for (int i = 0; i < m; i++)
    {
        cin >> s.items[i].pos.y >> s.items[i].pos.x >> s.items[i].type >> s.items[i].lifetime;
        // 注意：此时还没有读取蛇信息，所以不能使用s.self()
        switch (s.items[i].type)
        {
        case -1: // 成长食物
        {
            s.items[i].value = GROWTH_FOOD_VALUE;
            break;
        }

        case -2: // 陷阱 - 根据游戏规则扣除10分
            s.items[i].value = TRAP_PENALTY;
            break;
        case -3: // 钥匙
            s.items[i].value = KEY_VALUE;
            break;
        case -5: // 宝箱
            s.items[i].value = CHEST_VALUE;
            break;
        default: // 普通食物
        {
            int base_v = s.items[i].type;
            // 暂时使用基础值，稍后在读取完蛇信息后可以重新计算
            s.items[i].value = base_v;
        }
        break;
        }
    }

    // ========== 读取蛇信息?==========
    int ns;
    cin >> ns;
    s.snakes.resize(ns);
    unordered_map<int, int> id2idx; // ID到数组索引的映射
    id2idx.reserve(ns * 2);
    for (int i = 0; i < ns; i++)
    {
        auto &sn = s.snakes[i];
        cin >> sn.id >> sn.length >> sn.score >> sn.dir >> sn.shield_cd >> sn.shield_time;
        sn.body.resize(sn.length);
        for (int j = 0; j < sn.length; j++)
            cin >> sn.body[j].y >> sn.body[j].x;
        // 找到自己的蛇
        if (sn.id == MYID)
            s.self_idx = i; // 记录自己蛇的索引
        id2idx[sn.id] = i;
    }

    // 检查是否找到自己的蛇
    if (s.self_idx == -1)
    {
        // 如果没找到自己的蛇，可能已经死亡，输出默认动作
        cout << "0\n";
        cout << "|ERR:SNAKE_NOT_FOUND,MYID=" << MYID << "\n";
        exit(0);
    }

    // ========== 重新计算食物价值（现在可以使用 s.self()）==========
    const auto &me = s.self();
    for (auto &item : s.items)
    {
        if (item.type >= 1) // 普通食物?
        {
            int base_v = item.type;
            item.value = base_v;
            item.value *= NORMAL_FOOD_MULTIPLIER; // 应用新的价值调整?
        }
        else if (item.type == -1) // 成长食物 - 使用精确的价值计算?
        {
            item.value = calculate_growth_food_value(s.current_ticks, me.length, me.score);
        }
    }

    // ========== 读取宝箱信息 ==========
    int nc;
    cin >> nc;
    s.chests.resize(nc);
    for (int i = 0; i < nc; i++)
        cin >> s.chests[i].pos.y >> s.chests[i].pos.x >> s.chests[i].score;

    // After reading chests (keep your existing code to fill s.chests)
    for (const auto &chest : s.chests)
    {
        for (auto &it : s.items)
        {
            if (it.type == -5 && it.pos.y == chest.pos.y && it.pos.x == chest.pos.x)
            {
                it.value = chest.score * CHEST_SCORE_MULTIPLIER; // <-- use actual score
                break;
            }
        }
    }

    // ========== 读取钥匙信息 ==========
    int nk;
    cin >> nk;
    s.keys.resize(nk);
    for (int i = 0; i < nk; i++)
    {
        cin >> s.keys[i].pos.y >> s.keys[i].pos.x >> s.keys[i].holder_id >> s.keys[i].remaining_time;
        // 如果钥匙被某条蛇持有，标记该蛇
        if (s.keys[i].holder_id != -1)
        {
            auto it = id2idx.find(s.keys[i].holder_id);
            if (it != id2idx.end())
                s.snakes[it->second].has_key = true;
        }
    }

    // ========== 读取安全区域信息 ==========
    cin >> s.cur.x_min >> s.cur.y_min >> s.cur.x_max >> s.cur.y_max;
    cin >> s.next_tick >> s.next.x_min >> s.next.y_min >> s.next.x_max >> s.next.y_max;
    cin >> s.fin_tick >> s.fin.x_min >> s.fin.y_min >> s.fin.x_max >> s.fin.y_max;

    // consume trailing newline from the last numeric read
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
    string line;
    if (std::getline(cin, line))
    {
        str_info = line; // full memory payload (may be empty)
    }
    else
    {
        str_info.clear(); // first tick or no memory
    }
}

// ==================== 辅助函数 ====================

/**
 * 检查坐标是否在地图边界内?
 */
inline bool in_bounds(int y, int x)
{
    return (0 <= y && y < H && 0 <= x && x < W);
}

/**
 * 检查坐标是否在安全区域内（不包括地图边界检查）
 */
inline bool in_safe_zone(const Safe &z, int y, int x)
{
    return x >= z.x_min && x <= z.x_max && y >= z.y_min && y <= z.y_max;
}

/**
 * 若坐标在当前安全区内：
 *   1) 如果下次收缩不存在(next_tick == -1) 或距离收缩还有 > SAFE_ZONE_SHRINK_THRESHOLD 回合，返回-1
 *   2) 否则返回该点到“收缩后安全区域global_state.next) 的曼哈顿距离（若点已在下一安全区内则为0）?
 * 若坐标不在当前安全区内，返回 -1
 */
/**
 * 评估指定坐标在给定时间点的安全区危险等级 - 新的支持任意时间点检测的函数
 * 支持检测任意时间点的安全区域安全状态?
 *
 * 参数：?
 *   current_safe_zone: 当前安全区域
 *   next_safe_zone: 下一次收缩后的安全区域
 *   next_shrink_tick: 下一次收缩的时间点?(-1表示无收缩?
 *   current_tick: 要检测的时间点?
 *   y, x: 要检测的坐标
 *
 * 返回值：
 *   -1: 不在当前安全区内（立即危险）
 *   1-3: 低危险（安全）
 *   4-15: 高危险（需要尽快移动到安全位置）
 *
 * 使用示例：
 *   // 检测当前时间点
 *   int hazard_now = danger_safe_zone(game_state.cur, game_state.next,
 *                                    game_state.next_tick, game_state.current_ticks, y, x);
 *
 *   // 检测未来时间点（比较?回合后）
 *   int hazard_future = danger_safe_zone(game_state.cur, game_state.next,
 *                                        game_state.next_tick, game_state.current_ticks + 5, y, x);
 */
inline int danger_safe_zone(const Safe &current_safe_zone, const Safe &next_safe_zone,
                            int next_shrink_tick, int current_tick, int y, int x)
{
    // 不在当前安全区?- 立即危险
    if (!(x >= current_safe_zone.x_min && x <= current_safe_zone.x_max &&
          y >= current_safe_zone.y_min && y <= current_safe_zone.y_max))
        return -1;

    // 没有计划收缩 - 最低危险?
    if (next_shrink_tick == -1)
    {
        return 1; // 稳定安全区的最小危险级?
    }

    int ticks_until_shrink = next_shrink_tick - current_tick;

    // 短期内不会收缩?- 低危险?
    if (ticks_until_shrink > SAFE_ZONE_SHRINK_THRESHOLD)
    {
        return 2; // 非收缩区域的低危险级?
    }

    // 计算到收缩后安全区的曼哈顿距离（在内部为0）
    int dx = 0, dy = 0;
    if (x < next_safe_zone.x_min)
        dx = next_safe_zone.x_min - x;
    else if (x > next_safe_zone.x_max)
        dx = x - next_safe_zone.x_max;
    if (y < next_safe_zone.y_min)
        dy = next_safe_zone.y_min - y;
    else if (y > next_safe_zone.y_max)
        dy = y - next_safe_zone.y_max;

    int distance_to_next_zone = dx + dy;

    // 危险度计算?
    if (distance_to_next_zone == 0)
    {
        // 已在下个安全区内 - 低危险?
        if (ticks_until_shrink <= 1)
        {
            return 3; // 收缩临近时稍微提升的低危险?
        }
        else
        {
            return 2; // 低危险?- 已在下个安全区内
        }
    }
    else
    {
        // 不在下个安全区内 - 高危险?
        int time_pressure = max(1, SAFE_ZONE_SHRINK_THRESHOLD - ticks_until_shrink + 1);
        return min(15, 10 + distance_to_next_zone + time_pressure);
    }
}

/**
 * 兼容旧接口的函数 - 使用全局状态检测当前时间点的安全区危险
 */
inline int danger_safe_zone(const Safe &z, int y, int x)
{
    return danger_safe_zone(z, global_state.next, global_state.next_tick,
                            global_state.current_ticks, y, x);
}

// 游戏引擎动作映射 (按规范): 0=左,1=上,2=右,3=下,4=护盾
static const int DX[4] = {-1, 0, 1, 0}; // x方向偏移：左、上、右、下
static const int DY[4] = {0, -1, 0, 1}; // y方向偏移：左、上、右、下
static const int ACT[4] = {0, 1, 2, 3}; // 索引0..3 -> 动作代码

/**
 * 曼哈顿距离辅助函数（用于路径检查）
 */
inline int manhattan(int y1, int x1, int y2, int x2)
{
    return abs(y1 - y2) + abs(x1 - x2);
}

// ===== TTL软衰减函数 =====
// 当ETA超过生命周期时应用软衰减而不是硬丢弃
// 如果分数变得微不足道则返回false；否则返回true
static bool ttl_soft_decay(double &score, int dist, int lifetime,
                           int grace_ticks = TTL_DECAY_GRACE_TICKS,
                           double tau = TTL_DECAY_TAU)
{
    if (lifetime < 0)
        return true; // 永久物品
    int late = dist - (lifetime + grace_ticks);
    if (late <= 0)
        return true;
    score *= std::exp(-double(late) / tau);
    if (!std::isfinite(score) || score < 1e-6)
        return false;
    return true;
}

/**
 * 网格掩码结构
 * 使用位掩码高效存储地图状态?
 */
struct GridMask
{
    bitset<W> blocked_rows[H]; // 墙位置的位掩码
    bitset<W> snake_rows[H];   // 敌方蛇身体位置的位掩码?
    bitset<W> danger_rows[H];  // 危险位置的位掩码
    bitset<W> trap_rows[H];    // 陷阱位置的位掩码
    /**
     * 标记位置为墙
     */
    inline void block(int y, int x)
    {
        if (in_bounds(y, x))
            blocked_rows[y].set(x);
    }
    /**
     * 标记位置为敌方蛇身体
     */
    inline void snake(int y, int x)
    {
        if (in_bounds(y, x) && in_safe_zone(global_state.cur, y, x))
            snake_rows[y].set(x);
    }

    /**
     * 标记位置为危险?蛇头周围
     */
    inline void danger(int y, int x)
    {
        if (in_bounds(y, x) && in_safe_zone(global_state.cur, y, x))
            danger_rows[y].set(x);
    }
    /**
     * 标记位置为陷阱?
     */
    inline void trap(int y, int x)
    {
        if (in_bounds(y, x) && in_safe_zone(global_state.cur, y, x))
            trap_rows[y].set(x);
    }
    /**
     * 检查位置是否被阻挡
     */
    inline bool blocked(int y, int x) const
    {
        return (in_bounds(y, x) && in_safe_zone(global_state.cur, y, x)) ? blocked_rows[y].test(x) : true;
    }

    /**
     * 检查位置是否是敌方蛇身体?
     */
    inline bool is_snake(int y, int x) const
    {
        return (in_bounds(y, x) && in_safe_zone(global_state.cur, y, x)) ? snake_rows[y].test(x) : false;
    }

    /**
     * 检查位置是否危险?
     */
    inline bool is_danger(int y, int x) const
    {
        return (in_bounds(y, x) && in_safe_zone(global_state.cur, y, x)) ? danger_rows[y].test(x) : true;
    }

    /**
     * 检查位置是否为陷阱
     */
    inline bool is_trap(int y, int x) const
    {
        return (in_bounds(y, x) && in_safe_zone(global_state.cur, y, x)) ? trap_rows[y].test(x) : false;
    }
};

// ===== 未来安全区辅助函数 =====
// 一步后将生效的安全区域
static inline Safe zone_after_one_step(const State &s)
{
    if (s.next_tick != -1 && s.current_ticks + 1 >= s.next_tick)
        return s.next;
    return s.cur;
}

// 选择在未来安全区内的替代方向
static int pick_dir_staying_in_future_zone(const State &s,
                                           const GridMask &M,
                                           int sy, int sx,
                                           int current_dir,
                                           std::stringstream &log_ss)
{
    const Safe fut = zone_after_one_step(s);
    const int opposite = (s.self().dir + 2) % 4;

    for (int k = 0; k < 4; ++k)
    {
        if (!FUTURE_SAFE_FALLBACK_ALLOW_UTURN && k == opposite)
            continue;
        int ny = sy + DY[k], nx = sx + DX[k];
        if (!in_bounds(ny, nx))
            continue;
        if (M.blocked(ny, nx))
            continue;
        if (!in_safe_zone(fut, ny, nx))
            continue;
        log_ss << "FUTURE_SAFE_REPLACE_DIR=" << k << "|";
        return k;
    }
    return -1;
}

// 前向声明
int survival_strategy(const State &s, int sy, int sx, stringstream &log_ss, const GridMask &M);

// ==================== 路径选择偏好函数 ====================

/**
 * 计算位置的开阔度分数 - 检查周围可自由移动的空间?
 * @param M GridMask引用，用于检查障碍物
 * @param y, x 要检查的位置坐标
 * @return 开阔度分数（?-1之间）?
 */
double calculate_openness(const GridMask &M, int y, int x)
{
    int free_spaces = 0;
    int total_spaces = 0;

    // 检查周围OPENNESS_RADIUS范围内的空间
    for (int dy = -OPENNESS_RADIUS; dy <= OPENNESS_RADIUS; dy++)
    {
        for (int dx = -OPENNESS_RADIUS; dx <= OPENNESS_RADIUS; dx++)
        {
            if (dy == 0 && dx == 0)
                continue; // 跳过中心点?

            int ny = y + dy, nx = x + dx;
            if (in_bounds(ny, nx) && in_safe_zone(global_state.cur, ny, nx))
            {
                total_spaces++;
                if (!M.blocked(ny, nx) && !M.is_snake(ny, nx) && !M.is_trap(ny, nx) && !M.is_danger(ny, nx))
                {
                    free_spaces++;
                }
            }
        }
    }

    return total_spaces > 0 ? (double)free_spaces / total_spaces : 0.0;
}

/**
 * 输出当前的开阔程序?（无阻挡、无敌方蛇、无陷阱、无危险级?
 * @param M GridMask引用
 * @param y, x 要检查的位置坐标
 * @return 开阔程度（0-4之间）?
 */

int get_openness_score(const GridMask &M, int y, int x)
{
    int escape_routes = 0;

    // 检查四个基本方向?
    for (int k = 0; k < 4; k++)
    {
        int ny = y + DY[k], nx = x + DX[k];
        if (in_bounds(ny, nx) && in_safe_zone(global_state.cur, ny, nx) &&
            !M.blocked(ny, nx) && !M.is_danger(ny, nx))
        {
            escape_routes++;
        }
    }

    return escape_routes;
}

/**
 * 检查位置是否为死路 - 只有一个或零个逃生方向
 * @param M GridMask引用
 * @param y, x 要检查的位置坐标
 * @return true如果是死路?
 */
bool is_dead_end(const GridMask &M, int y, int x)
{
    int escape_routes = get_openness_score(M, y, x);
    return escape_routes < MIN_ESCAPE_ROUTES;
}

/**
 * 检查路径是否过于狭窄?- 缺乏机动空间
 * @param M GridMask引用
 * @param y, x 要检查的位置坐标
 * @return true如果路径狭窄
 */
bool is_narrow_path(const GridMask &M, int y, int x)
{

    int extended_free = 0;
    int extended_total = 0;

    // 检查?步半径范围内的活动空间?
    for (int dy = -2; dy <= 2; dy++)
    {
        for (int dx = -2; dx <= 2; dx++)
        {
            if (abs(dy) + abs(dx) > 2)
                continue;
            if (dy == 0 && dx == 0)
                continue;

            int ny = y + dy, nx = x + dx;
            if (in_bounds(ny, nx) && in_safe_zone(global_state.cur, ny, nx))
            {
                extended_total++;
                if (!M.blocked(ny, nx) && !M.is_snake(ny, nx) && !M.is_trap(ny, nx) && !M.is_danger(ny, nx))
                {
                    extended_free++;
                }
            }
        }
    }

    double openness_ratio = (double)extended_free / (extended_total + 1);
    return openness_ratio < 0.6;
}

/**
 * 检查位置是否靠近安全区边界
 * @param y, x 要检查的位置坐标
 * @return true如果靠近边界
 */
bool near_safe_zone_boundary(int y, int x)
{
    const Safe &zone = global_state.cur;
    int dist_to_boundary = min({x - zone.x_min,
                                zone.x_max - x,
                                y - zone.y_min,
                                zone.y_max - y});

    return dist_to_boundary <= SAFE_ZONE_BOUNDARY_THRESHOLD;
}

/**
 * 计算路径的综合危险度惩罚（带距离信息）?
 * @param M GridMask引用
 * @param y, x 位置坐标
 * @param expected_reward 预期奖励
 * @param distance_to_target 到达目标的距离（-1表示未知）?
 * @return 惩罚分数（正数表示惩罚）
 */
int calculate_path_danger_penalty(const GridMask &M, int y, int x, int expected_reward, int distance_to_target)
{
    int penalty = 0;

    // 死路检查?
    if (is_dead_end(M, y, x))
    {
        penalty += DEAD_END_PENALTY;
        // 如果奖励很低，额外加重惩罚?
        if (expected_reward < MIN_REWARD_THRESHOLD)
        {
            penalty += DEAD_END_PENALTY;
        }
    }

    // 增强的狭窄路径惩罚，考虑蛇长度因素?
    if (is_narrow_path(M, y, x))
    {
        const auto &me = global_state.self();
        int base_penalty = NARROW_PATH_PENALTY;

        penalty += base_penalty;

        // 如果奖励低，额外惩罚
        if (expected_reward < MIN_REWARD_THRESHOLD)
        {
            penalty += base_penalty / 2;
        }

        // 游戏后期空间更关键时的额外惩罚?
        if (global_state.current_ticks > LATE_GAME_THRESHOLD) // 游戏后期
        {
            penalty += base_penalty;
        }
    }

    // 增强的安全区边界检查，具有适当的危险等级?
    int safe_zone_hazard = danger_safe_zone(global_state.cur, y, x);
    if (safe_zone_hazard > 0)
    {
        if (safe_zone_hazard >= 8) // 高危险（收缩区域）?
        {
            // 对收缩安全区位置的显著惩罚?
            penalty += SAFE_ZONE_BOUNDARY_PENALTY * (safe_zone_hazard - 7); // 根据危险等级缩放
            if (expected_reward < MIN_REWARD_THRESHOLD)
            {
                penalty += SAFE_ZONE_BOUNDARY_PENALTY * 2; // 低奖励路径双倍惩罚?
            }
        }
        else if (safe_zone_hazard >= 1 && safe_zone_hazard <= 2) // 低危险（稳定区域）?
        {
            // 稳定安全区的最小惩罚?- 只是轻微谨慎
            penalty += SAFE_ZONE_BOUNDARY_PENALTY / 4; // 非常轻的惩罚
        }
        else
        {
            penalty += SAFE_ZONE_BOUNDARY_PENALTY / 2;
        }
    }

    // === 新增：吃食物后安全区收缩风险检查?===
    // 检查吃完食物后是否会因安全区收缩而处于危险位置?
    if (global_state.next_tick != -1)
    {
        // 计算蛇到达目标位置的时间
        int arrival_time;
        if (distance_to_target >= 0)
        {
            arrival_time = global_state.current_ticks + distance_to_target; // 使用精确距离
        }
        else
        {
            arrival_time = global_state.current_ticks + 1; // 简化估计?
        }
        int post_eating_time = arrival_time + 1; // 吃食物需要额外?回合

        // 如果吃完食物后安全区已经收缩
        if (post_eating_time >= global_state.next_tick)
        {
            // 检查该位置在收缩后是否还安全?
            if (!in_safe_zone(global_state.next, y, x))
            {
                // 根据时间紧迫性应用惩罚?
                int time_until_shrink = global_state.next_tick - global_state.current_ticks;
                int urgency_penalty = SAFE_ZONE_BOUNDARY_PENALTY * 3; // 基础惩罚

                // 收缩越紧迫，惩罚越重
                if (time_until_shrink <= 2)
                {
                    urgency_penalty *= 2; // 非常紧急?
                }
                else if (time_until_shrink <= 5)
                {
                    urgency_penalty = urgency_penalty * 3 / 2; // 比较紧急?
                }

                penalty += urgency_penalty;
            }
        }
    }

    return penalty;
}

/**
 * 计算路径的综合危险度惩罚
 * @param M GridMask引用
 * @param y, x 位置坐标
 * @param expected_reward 预期奖励
 * @return 惩罚分数（正数表示惩罚）
 */
int calculate_path_danger_penalty(const GridMask &M, int y, int x, int expected_reward)
{
    return calculate_path_danger_penalty(M, y, x, expected_reward, -1); // -1表示未知距离
}

/**
 * 应用开阔区域奖励到分数
 * @param M GridMask引用
 * @param y, x 位置坐标
 * @param base_score 基础分数
 * @return 调整后的分数
 */
double apply_openness_bonus(const GridMask &M, int y, int x, double base_score)
{
    double openness = calculate_openness(M, y, x);
    double bonus_multiplier = 1.0 + (openness * (OPEN_AREA_BONUS - 1.0));
    return base_score * bonus_multiplier;
}

/**
 * 计算位置周围的食物密度并返回密度奖励倍数
 * @param s 游戏状态?
 * @param y, x 要检查的位置坐标
 * @return 食物密度奖励倍数（?.0表示无奖励，更高表示有奖励）
 */
double calculate_food_density_bonus(const State &s, int y, int x)
{
    int food_count = 0;
    int total_food_value = 0;

    // 计算指定半径内的食物数量和总价值?
    for (const auto &item : s.items)
    {
        // 只计算有益的食物
        if (item.type >= 1 || item.type == -1 || item.type == -3 || item.type == -5)
        {
            int dist = manhattan(y, x, item.pos.y, item.pos.x);
            if (dist <= FOOD_DENSITY_RADIUS)
            {
                food_count++;
                // Use actual item value for consistent food density calculation
                total_food_value += item.value;
            }
        }
    }

    // 如果食物数量不足，不给予奖励
    if (food_count < MIN_FOOD_COUNT_FOR_BONUS)
        return 0.8;

    // 计算密度奖励：基于食物数量和价值?
    double density_factor = min(1.0, (double)food_count / 6.0);       // 6个食物为满密�?
    double value_factor = min(1.0, (double)total_food_value / 100.0); // 100分为满价值?

    // 综合密度和价值因素?
    double combined_factor = (density_factor + value_factor) / 2.0;

    // 应用奖励
    double bonus_multiplier = 1.0 + (combined_factor * FOOD_DENSITY_BONUS_BASE);

    // 限制最大奖励?
    return min(bonus_multiplier, FOOD_DENSITY_BONUS_MAX);
}

/**
 * 预测多步碰撞风险 - 更合理的版本
 * @param s 游戏状态?
 * @param my_target 我的目标位置
 * @param my_dist 我到目标的距离?
 * @return 碰撞风险评分数?.0-1.0，越高越危险级?
 */
double predict_collision_risk(const State &s, const Point &my_target, int my_dist)
{
    const auto &me = s.self();
    double total_risk = 0.0;
    int nearby_enemies = 0;

    for (const auto &enemy : s.snakes)
    {
        if (enemy.id == MYID)
            continue;

        Point enemy_head = enemy.head();

        // 只检查距离很近的敌人
        int dist_to_enemy = manhattan(me.head().y, me.head().x, enemy_head.y, enemy_head.x);
        if (dist_to_enemy > 4)
            continue; // 距离超过4格的敌人不考虑

        // 检查敌人是否也在朝我的目标移动
        int enemy_dist_to_my_target = manhattan(enemy_head.y, enemy_head.x, my_target.y, my_target.x);

        // 只有当敌人也可能在争夺同一目标时才考虑碰撞风险
        if (abs(enemy_dist_to_my_target - my_dist) <= 1)
        {
            nearby_enemies++;

            // 敌人越近，碰撞几率指数上�?
            const double distance_factor = max(1, dist_to_enemy);
            const double exponential_risk = 1.0 / (distance_factor * distance_factor); // 距离的平方反�?

            // 基于目标争夺的额外风�?
            const double target_competition_risk = 1.0 - (double)enemy_dist_to_my_target / 6.0;

            // 组合风险计算
            const double individual_risk = min(1.0, exponential_risk + target_competition_risk * 0.3);
            total_risk += individual_risk;
        }
    }

    // 敌人越多，总体风险递增（非线性增长）
    if (nearby_enemies > 0)
    {
        const double enemy_count_multiplier = 1.0 + (nearby_enemies - 1) * 0.4; // 每多一个敌人增�?0%风险
        total_risk *= enemy_count_multiplier;
    }

    return min(1.0, total_risk); // 确保风险不超�?.0
}

// ---------- Canonical BFS output (arrays + direction parent) ----------
struct BFSOut
{
    int dist[H][W];       // shortest cost
    int parent[H][W];     // direction FROM parent TO (y,x); -1 means none
    int snake_cost[H][W]; // optional, currently 0

    BFSOut()
    {
        for (int i = 0; i < H; ++i)
            for (int j = 0; j < W; ++j)
                dist[i][j] = INF_DIST, parent[i][j] = -1, snake_cost[i][j] = 0;
    }
};

// Reconstruct first step direction from BFS parent chain
static int reconstruct_first_step_dir(const BFSOut &G, int sy, int sx, int ty, int tx)
{
    if (sy == ty && sx == tx)
        return -1; // already there
    if (G.dist[ty][tx] >= INF_DIST)
        return -1; // unreachable

    // If target is adjacent, we can directly return the direction
    if (G.dist[ty][tx] == 1)
    {
        return G.parent[ty][tx];
    }

    // Walk back from target using parent directions
    int cy = ty, cx = tx;
    while (G.dist[cy][cx] > 1)
    {
        int parent_dir = G.parent[cy][cx];
        if (parent_dir == -1)
        {
            return -1; // broken chain
        }
        int parent_y = cy - DY[parent_dir];
        int parent_x = cx - DX[parent_dir];
        cy = parent_y;
        cx = parent_x;
    }

    // Now cy,cx should be distance 1 from start, return the direction from start to cy,cx
    return G.parent[cy][cx];
}

// Dijkstra on grid with soft penalties; forbids U-turn only on the first step
BFSOut bfs_grid(const GridMask &M, const State &s, int sy, int sx,
                const Snake *snake_for_pathfinding = nullptr)
{
    BFSOut G;

    const Snake &path_snake = snake_for_pathfinding ? *snake_for_pathfinding : s.self();
    const int opposite_dir = (path_snake.dir + 2) % 4;

    using Node = tuple<int, int, int>; // (cost, y, x)
    priority_queue<Node, vector<Node>, greater<Node>> pq;

    auto push = [&](int d, int y, int x, int from_dir)
    {
        if (d < G.dist[y][x])
        {
            G.dist[y][x] = d;
            G.parent[y][x] = from_dir; // dir from parent -> (y,x)
            pq.emplace(d, y, x);
        }
    };

    G.dist[sy][sx] = 0;
    pq.emplace(0, sy, sx);

    while (!pq.empty())
    {
        auto [cd, cy, cx] = pq.top();
        pq.pop();
        if (cd != G.dist[cy][cx])
            continue; // stale

        for (int k = 0; k < 4; ++k)
        {
            int ny = cy + DY[k], nx = cx + DX[k];
            if (!in_bounds(ny, nx))
                continue;
            if (!in_safe_zone(s.cur, ny, nx))
                continue; // outside safe zone blocked
            if (M.blocked(ny, nx))
                continue;

            // forbid U-turn only for the very first step from the source
            if (cy == sy && cx == sx && k == opposite_dir)
                continue;

            int step = 1;
            if (M.is_trap(ny, nx))
                step += TRAP_STEP_COST;

            bool near_snake = false;
            for (int t = 0; t < 4; ++t)
            {
                int ay = ny + DY[t], ax = nx + DX[t];
                if (in_bounds(ay, ax) && M.is_snake(ay, ax))
                {
                    near_snake = true;
                    break;
                }
            }
            if (near_snake)
                step += NEAR_ENEMY_ADJ_PENALTY;

            int nd = cd + step;
            push(nd, ny, nx, k);
        }
    }
    return G;
}

// 优势程度计算 - 分析我方相对于敌方在争夺目标时的优劣�?
double calculate_advantage(const Snake &me, const Snake &enemy, const Point &target, int my_dist, int enemy_dist, bool i_win_tie)
{
    double advantage = 0.0;

    // 距离优势：敌人距离越远，我方优势越大（每格差�?分）
    advantage += (enemy_dist - my_dist) * 2.0;

    // 护盾优势：拥有更长护盾时间的一方占�?
    if (me.shield_time > enemy.shield_time)
        advantage += 3.0; // 我方护盾时间更长，加3分优化?
    else if (me.shield_time < enemy.shield_time)
        advantage -= 3.0; // 敌方护盾时间更长，减3分优化?

    // 护盾开启能力优势：能开护盾而对方不能时占优
    if (can_open_shield(me) && !can_open_shield(enemy))
        advantage += 2.0; // 我方能开护盾而敌方不能，�?�?
    else if (!can_open_shield(me) && can_open_shield(enemy))
        advantage -= 2.0; // 敌方能开护盾而我方不能，�?�?

    // 平局处理：距离相等时根据平局规则决定优势
    if (my_dist == enemy_dist)
        advantage += i_win_tie ? 1.0 : -1.0; // 平局时我方获胜?1分，否则-1�?

    return advantage;
}

// 竞争检测函数?- 分析与敌方蛇争夺各种目标的优劣势
// 现在支持：食物、成长食物、钥匙、宝箱（需要钥匙时间?
vector<CompetitionAnalysis> analyze_competition(const State &s, const GridMask &M, const BFSOut &G)
{
    vector<CompetitionAnalysis> competitions;
    const auto &me = s.self();

    // 遍历所有游戏道具进行竞争分数?
    for (const auto &item : s.items)
    {
        // 扩展竞争分析：包含食物、成长食物、钥匙、宝箱?
        // 跳过陷阱(-2)，因为陷阱是有害的，不需要竞争分数?
        if (item.type == -2) // 只跳过陷阱?
            continue;

        // 对于宝箱，只有当我们有钥匙时才进行竞争分数?
        if (item.type == -5 && !me.has_key)
            continue;

        Point target = item.pos;
        int my_dist = G.dist[target.y][target.x];

        // 如果目标不可达，跳过此目�?
        if (my_dist >= INF_DIST)
            continue;

        // 分析与每个敌方蛇的竞争情�?
        for (const auto &enemy : s.snakes)
        {
            if (enemy.id == MYID)
                continue;

            // 简化竞争分析：直接使用曼哈顿距离计算?
            Point enemy_head = enemy.head();
            int enemy_dist = manhattan(enemy_head.y, enemy_head.x, target.y, target.x);

            // 创建竞争分析结果
            CompetitionAnalysis comp;
            comp.target = target;
            comp.my_dist = my_dist;
            comp.enemy_dist = enemy_dist;
            comp.enemy_id = enemy.id;
            comp.i_win_tie = determine_tie_winner(me, enemy, target, my_dist, enemy_dist);
            comp.advantage = calculate_advantage(me, enemy, target, my_dist, enemy_dist, comp.i_win_tie);
            competitions.push_back(comp);
        }
    }
    return competitions;
}

// 完整的路径构建算法?- 基于贪心策略的动态路径规�?
void build_route_greedy(const GridMask &M, const State &s, const vector<Cand> &C, Point head, Route &R)
{
    // 初始化路径结构
    R.seq.clear();  // 清空路径序列
    R.totalVal = 0; // 重置总价值
    R.finishT = 0;  // 重置完成时间

    // 如果没有候选目标，直接返回空路径
    if (C.empty())
        return;

    Point cur_pos = head;               // 当前位置（从蛇头开始）
    int cur_time = 0;                   // 当前累计时间
    vector<bool> used(C.size(), false); // 标记已使用的候选目标

    // 使用优先队列思想进行贪心选择最优目标
    // 每一步都重新评估所有剩余目标，选择当前最优的
    while (true)
    {
        int best_idx = -1;        // 最佳候选目标索引
        double best_score = -1e9; // 最佳评分（初始化为极小值）

        // 计算当前位置到所有未访问目标的评分
        for (int i = 0; i < C.size(); i++)
        {
            if (used[i])
                continue; // 跳过已使用的目标

            const auto &cand = C[i];
            // 从当前位置重新计算到候选目标的路径
            BFSOut path = bfs_grid(M, s, cur_pos.y, cur_pos.x);
            int d = path.dist[cand.p.y][cand.p.x];

            // 可达性检查：目标必须可达
            if (d >= INF_DIST)
                continue;

            // 时间限制检查：确保在目标消失前能够到达
            if (cand.ttl != -1 && cur_time + d > cand.ttl)
                continue;

            // 安全区收缩检查：确保目标在安全区收缩后仍然有安全性
            bool valid = true;
            if (s.next_tick != -1 && cur_time + d >= s.next_tick)
            {
                if (!in_safe_zone(s.next, cand.p.y, cand.p.x))
                {
                    valid = false; // 目标将在安全区外，无安全性
                }
            }

            if (!valid)
                continue;

            // 计算价值?距离比作为基础优先级评分?
            // +1.0避免除零，距离越近评分越�?
            double score = (double)cand.val / (d + 1.0);

            // 时间紧迫性加权：TTL越小，优先级越高
            // 这确保即将消失的高价值目标获得更高优先级
            if (cand.ttl != -1)
            {
                int remaining_time = cand.ttl - cur_time;
                if (remaining_time > 0)
                {
                    // 时间越紧迫，分数加成越大（反比关系）
                    score *= (1.0 + 10.0 / remaining_time);
                }
            }

            // 更新最佳选择
            if (score > best_score)
            {
                best_score = score;
                best_idx = i;
            }
        }

        // 终止条件：没有找到可行目标，结束路径构建
        if (best_idx == -1)
            break;

        // 选择最优目标并添加到路径?
        const auto &best_cand = C[best_idx];
        BFSOut path = bfs_grid(M, s, cur_pos.y, cur_pos.x);
        int d = path.dist[best_cand.p.y][best_cand.p.x];

        // 更新路径信息
        R.seq.push_back(best_cand.p); // 添加目标到路径序�?
        R.totalVal += best_cand.val;  // 累加目标价值?
        cur_time += d;                // 累加到达时间
        cur_pos = best_cand.p;        // 更新当前位置
        used[best_idx] = true;        // 标记目标为已使用

        // 长度限制：防止路径过长导致计算超�?
        // 8个目标通常足够一个回合的规划
        if (R.seq.size() >= 8)
            break;
    }

    R.finishT = cur_time; // 设置路径总完成时间?
}

// 路径改进算法 - 局部搜索优化路径顺序
void improve_route_ls(const GridMask &M, const State &s, Point head, Route &R)
{

    // 短路径无需优化：少于两个目标时直接返回
    if (R.seq.size() <= 2)
        return;
    // 但限制计算量以保持实时性能要求

    if (R.seq.size() >= 3)
    {
        // 策略：保持第一个目标不变，对剩余目标重新排�?
        // 这基于假设第一个目标是当前最重要/最紧急的
        Point first_target = R.seq[0];

        // 从第一个目标位置计算到其他目标的实际距离?
        // 这比从起点计算更准确，因为考虑了实际的游戏状态?
        BFSOut path_from_first = bfs_grid(M, s, first_target.y, first_target.x);

        // 构建剩余目标的候选列表，按实际距离排序
        vector<pair<int, int>> remaining; // 存储 (实际距离, 原索引)
        for (int i = 1; i < (int)R.seq.size(); i++)
        {
            int actual_dist = path_from_first.dist[R.seq[i].y][R.seq[i].x];
            // 如果无法到达，使用一个很大的距离值作为惩罚
            if (actual_dist >= INF_DIST)
            {
                actual_dist = 9999;
            }
            remaining.push_back({actual_dist, i});
        }

        // 按实际距离排序：距离近的目标优先访问
        // 这样可以减少总的移动时间，提高效率
        sort(remaining.begin(), remaining.end());

        // 重建路径序列：第一个目标 + 按距离排序的剩余目标
        vector<Point> new_seq;
        new_seq.push_back(first_target);
        for (const auto &item : remaining)
        {
            new_seq.push_back(R.seq[item.second]);
        }
        R.seq = new_seq;

        // 重新计算总时间（使用准确的BFS计算法?
        // 这确保路径时间估算的准确�?
        int total_time = 0;
        Point cur_pos = head;
        for (const auto &target : R.seq)
        {
            BFSOut path = bfs_grid(M, s, cur_pos.y, cur_pos.x);
            int d = path.dist[target.y][target.x];

            // 安全检查：如果某个目标不可达，保持原路径?
            if (d >= INF_DIST)
            {
                return; // 回退到原始路径?
            }
            total_time += d;
            cur_pos = target; // 更新当前位置
        }
        R.finishT = total_time; // 更新路径总完成时间
    }
}

// 统一的紧急处理：在安全区外时优先回到安全区，否则（无路且无法开盾）执行"拒绝投食"
static int emergency_handle_outside_safe(const State &s, stringstream &log_ss, const GridMask &M)
{
    const Snake &me = s.self();
    const int sy = me.head().y, sx = me.head().x;
    const int opposite = (me.dir + 2) % 4;

    // 1) 可开盾：立即开盾抢救（一次机会）
    if (me.shield_time == 0 && can_open_shield(me))
    {
        log_ss << "HEAD_OUTSIDE_SAFE:OPEN_SHIELD|";
        str_info += log_ss.str();
        return 4;
    }

    // 2) 选择"最少步数进入当前安全区"的方向（避免掉头/越界/阻塞/明显危险级?
    int best_dir = -1, best_steps = INT_MAX;
    for (int k = 0; k < 4; ++k)
    {
        if (k == opposite)
            continue;
        int ny = sy + DY[k], nx = sx + DX[k];
        if (!in_bounds(ny, nx))
            continue;
        if (M.blocked(ny, nx))
            continue;
        // 估算法?next cell)到进入安全区的曼哈顿"外距"
        int dx = 0, dy = 0;
        if (nx < s.cur.x_min)
            dx = s.cur.x_min - nx;
        else if (nx > s.cur.x_max)
            dx = nx - s.cur.x_max;
        if (ny < s.cur.y_min)
            dy = s.cur.y_min - ny;
        else if (ny > s.cur.y_max)
            dy = ny - s.cur.y_max;
        const int steps = dx + dy;
        if (steps < best_steps)
        {
            best_steps = steps;
            best_dir = k;
        }
        log_ss << "RET_SAFE_DIR" << k << ":(" << ny << "," << nx << ")steps=" << steps << "|";
    }
    if (best_dir != -1)
    {
        log_ss << "HEAD_OUTSIDE_SAFE:RETURN_DIR=" << best_dir << "|";
        return ACT[best_dir];
    }

    // 3) 绝望策略：远离敌人，避免"喂分"给最近的对手
    int best_suicide_dir = -1, farthest_enemy = -1;
    for (int k = 0; k < 4; ++k)
    {
        if (k == opposite)
            continue;
        int ny = sy + DY[k], nx = sx + DX[k];
        if (!in_bounds(ny, nx))
            continue;
        if (M.blocked(ny, nx))
            continue;
        int dmin = INT_MAX;
        for (const auto &e : s.snakes)
        {
            if (e.id == MYID)
                continue;
            dmin = min(dmin, manhattan(ny, nx, e.head().y, e.head().x));
        }
        if (dmin > farthest_enemy)
        {
            farthest_enemy = dmin;
            best_suicide_dir = k;
        }
    }
    log_ss << "DENY_FEEDING_STRATEGY:DIR=" << best_suicide_dir << ",enemy_dist=" << farthest_enemy << "|";
    return (best_suicide_dir != -1) ? ACT[best_suicide_dir] : 0;
}

// ===== A* shortest path (first step only) =====
int astar_first_step(const GridMask &M, const State &s, const Point &start, const Point &goal)
{
    if (start.y == goal.y && start.x == goal.x)
        return -1;

    static int dist[H][W];
    static signed char parent[H][W];
    for (int y = 0; y < H; ++y)
    {
        for (int x = 0; x < W; ++x)
        {
            dist[y][x] = INT_MAX;
            parent[y][x] = -1;
        }
    }

    struct Node
    {
        int f, g, y, x;
    };
    auto cmp = [](const Node &a, const Node &b)
    { return a.f > b.f; };
    std::priority_queue<Node, std::vector<Node>, decltype(cmp)> pq(cmp);
    auto h = [&](int y, int x)
    { return abs(y - goal.y) + abs(x - goal.x); };

    dist[start.y][start.x] = 0;
    pq.push({h(start.y, start.x), 0, start.y, start.x});

    while (!pq.empty())
    {
        Node cur = pq.top();
        pq.pop();
        if (cur.y == goal.y && cur.x == goal.x)
            break;
        if (cur.g != dist[cur.y][cur.x])
            continue;

        for (int k = 0; k < 4; ++k)
        {
            int ny = cur.y + DY[k], nx = cur.x + DX[k];
            if (ny < 0 || ny >= H || nx < 0 || nx >= W)
                continue;
            if (!in_safe_zone(s.cur, ny, nx))
                continue;
            if (M.blocked(ny, nx))
                continue;
            int step = 1;
            if (M.danger_rows[ny].test(nx))
                step += (s.self().shield_time > 0 ? 1 : 5);
            if (M.trap_rows[ny].test(nx))
                step += 8;
            int g2 = cur.g + step;
            if (g2 < dist[ny][nx])
            {
                dist[ny][nx] = g2;
                parent[ny][nx] = k;
                pq.push({g2 + h(ny, nx), g2, ny, nx});
            }
        }
    }
    if (dist[goal.y][goal.x] == INT_MAX)
        return -1;

    // Walk back from goal to start to find the **first step** dir
    int y = goal.y, x = goal.x, dir_from_prev = -1;
    while (!(y == start.y && x == start.x))
    {
        int d = parent[y][x];
        if (d == -1)
            return -1;
        int py = y - DY[d], px = x - DX[d];
        dir_from_prev = d;
        y = py;
        x = px;
    }
    return dir_from_prev;
}

// ===== A*算法用于单目标第一步提取 =====
// 返回方向0..3，如果不可达则返回-1。使用与Dijkstra相同的步骤代价模型。
int astar_first_step_detailed(const GridMask &M,
                              const State &s,
                              Point start,
                              Point goal,
                              const Snake *snake_for_pathfinding = nullptr)
{
    if (!in_bounds(goal.y, goal.x))
        return -1;
    if (start.y == goal.y && start.x == goal.x)
        return -1;

    static int g[H][W];
    static int parent_dir[H][W];
    for (int y = 0; y < H; ++y)
        for (int x = 0; x < W; ++x)
            g[y][x] = INF_DIST, parent_dir[y][x] = -1;

    const Snake &ps = snake_for_pathfinding ? *snake_for_pathfinding : s.self();
    const int opposite = (ps.dir + 2) % 4;

    auto h = [&](int y, int x)
    { return std::abs(y - goal.y) + std::abs(x - goal.x); };

    auto step_cost = [&](int y, int x)
    {
        int step = 1;
        if (M.is_trap(y, x))
            step += TRAP_STEP_COST;
        bool near_snake = false;
        for (int t = 0; t < 4; ++t)
        {
            int ay = y + DY[t], ax = x + DX[t];
            if (in_bounds(ay, ax) && M.is_snake(ay, ax))
            {
                near_snake = true;
                break;
            }
        }
        if (near_snake)
            step += NEAR_ENEMY_ADJ_PENALTY;
        return step;
    };

    using Node = std::tuple<int, int, int>; // (f, y, x)
    std::priority_queue<Node, std::vector<Node>, std::greater<Node>> pq;

    g[start.y][start.x] = 0;
    pq.emplace(h(start.y, start.x), start.y, start.x);

    while (!pq.empty())
    {
        auto [fcur, cy, cx] = pq.top();
        pq.pop();
        if (cy == goal.y && cx == goal.x)
            break;

        for (int k = 0; k < 4; ++k)
        {
            int ny = cy + DY[k], nx = cx + DX[k];
            if (!in_bounds(ny, nx))
                continue;
            if (!in_safe_zone(s.cur, ny, nx))
                continue; // 遍历合法性使用当前区域
            if (M.blocked(ny, nx))
                continue;
            if (cy == start.y && cx == start.x && k == opposite)
                continue; // 避免立即U型转弯

            int nd = g[cy][cx] + step_cost(ny, nx);
            if (nd < g[ny][nx])
            {
                g[ny][nx] = nd;
                parent_dir[ny][nx] = k; // 边方向：(cy,cx) -> (ny,nx)
                pq.emplace(nd + h(ny, nx), ny, nx);
            }
        }
    }

    if (g[goal.y][goal.x] >= INF_DIST)
        return -1;

    // 回溯一条边获取第一步
    int ty = goal.y, tx = goal.x;
    while (!(ty == start.y && tx == start.x))
    {
        int k = parent_dir[ty][tx];
        if (k < 0)
            return -1;
        int py = ty - DY[k], px = tx - DX[k];
        if (py == start.y && px == start.x)
            return k;
        ty = py;
        tx = px;
    }
    return -1;
}

// Prefer landing inside next zone when shrink is imminent (<=2 ticks)
int enforce_future_safezone_or_fallback(const State &s, const GridMask &M,
                                        int sy, int sx, int dir, std::stringstream &log_ss,
                                        int ticks_guard = 2)
{
    auto ok = [&](int y, int x)
    {
        if (y < 0 || y >= H || x < 0 || x >= W)
            return false;
        if (!in_safe_zone(s.cur, y, x))
            return false;
        if (M.blocked(y, x))
            return false;
        return true;
    };
    int ny = sy + DY[dir], nx = sx + DX[dir];
    bool shrink_imminent = (s.next_tick != -1) && (s.next_tick - s.current_ticks <= ticks_guard);

    if (!ok(ny, nx))
    {
        for (int k = 0; k < 4; ++k)
        {
            if (k == (s.self().dir + 2) % 4)
                continue;
            int ty = sy + DY[k], tx = sx + DX[k];
            if (ok(ty, tx))
            {
                log_ss << "OVERRIDE_DIR:" << k << "|";
                return k;
            }
        }
        return dir;
    }
    if (!shrink_imminent)
        return dir;
    if (in_safe_zone(s.next, ny, nx))
        return dir;

    int best = -1, best_h = INT_MAX;
    for (int k = 0; k < 4; ++k)
    {
        if (k == (s.self().dir + 2) % 4)
            continue;
        int ty = sy + DY[k], tx = sx + DX[k];
        if (!ok(ty, tx))
            continue;
        if (in_safe_zone(s.next, ty, tx))
        {
            int h = abs(ty - s.next.y_min) + abs(tx - s.next.x_min); // cheap heuristic
            if (h < best_h)
            {
                best_h = h;
                best = k;
            }
        }
    }
    if (best != -1)
    {
        log_ss << "SAFEZONE_OVERRIDE:" << best << "|";
        return best;
    }
    return dir;
}

// 在返回动作前强制执行未来安全区
// 如果下一个位置在缩圈后会处于安全区外，尝试替代方向，否则护盾，否则生存回退。
static int enforce_future_safezone_or_fallback_detailed(const State &s,
                                                        const GridMask &M,
                                                        int sy, int sx,
                                                        int dir,
                                                        std::stringstream &log_ss)
{
    Safe fut = zone_after_one_step(s);
    int ny = sy + DY[dir], nx = sx + DX[dir];

    if (in_bounds(ny, nx) && in_safe_zone(fut, ny, nx))
        return dir; // 缩圈后安全

    log_ss << "FUTURE_SAFE_BLOCK_DIR=" << dir << "|";

    if (int alt = pick_dir_staying_in_future_zone(s, M, sy, sx, dir, log_ss); alt != -1)
        return alt;

    const Snake &me = s.self();
    if (me.shield_time == 0 && can_open_shield(me))
    {
        log_ss << "FUTURE_SAFE_USE_SHIELD|";
        return 4; // 护盾
    }

    log_ss << "FUTURE_SAFE_SURVIVAL|";
    return survival_strategy(s, sy, sx, log_ss, const_cast<GridMask &>(M));
}

// 完整的生存策略函数?- 当AI无法找到食物目标时的紧急生存逻辑
// 用于处理没有明确目标时的移动决策，支持多层次的安全评估和生存策略
int survival_strategy(const State &s, int sy, int sx, stringstream &log_ss, const GridMask &M)
{
    log_ss << "SURVIVAL_STRATEGY_ENTERED:pos=(" << sy << "," << sx << ")|";

    // Enhanced fallback strategy for large snakes - prioritize positioning and space
    const auto &me = s.self();
    int opposite_dir_basic = (me.dir + 2) % 4;
    int best_basic_dir = -1;
    double best_basic_score = -1000.0;

    for (int k = 0; k < 4; ++k)
    {
        if (k == opposite_dir_basic)
            continue; // 不掉�?

        int ny = sy + DY[k], nx = sx + DX[k];

        if (!in_bounds(ny, nx) || !in_safe_zone(s.cur, ny, nx))
            continue;

        if (M.blocked(ny, nx))
            continue;

        // 计算这个方向的综合评分?
        double safety_score = 100.0;

        // 检查是否有蛇身碰撞风险（无护盾时）
        if (M.is_snake(ny, nx) && me.shield_time == 0)
            continue; // 跳过会撞蛇的方向

        // 添加食物密度奖励到基本移动评分?
        double food_density_bonus = calculate_food_density_bonus(s, ny, nx);
        safety_score *= food_density_bonus;

        // 给予不错的评分，优先选择这些安全方向
        if (safety_score > best_basic_score)
        {
            best_basic_score = safety_score;
            best_basic_dir = k;
        }
    }

    if (best_basic_dir != -1)
    {
        log_ss << "BASIC_SAFE_MOVE:DIR" << best_basic_dir << ",score=" << best_basic_score << "|";
        return ACT[best_basic_dir];
    }

    log_ss << "SURVIVAL_MODE:|";

    // 寻找可达性最好的安全移动方向
    int bestDir = -1;
    int bestReach = -1;

    // 禁止掉头
    int opposite_dir = (me.dir + 2) % 4;

    for (int k = 0; k < 4; ++k)
    {
        int ny = sy + DY[k], nx = sx + DX[k];

        log_ss << "SURVIVAL_CHECK_DIR" << k << ":pos=(" << ny << "," << nx << ")";

        // 防止掉头
        if (k == opposite_dir)
        {
            log_ss << ",SKIP_OPPOSITE|";
            continue;
        }

        if (!in_bounds(ny, nx))
        {
            log_ss << ",OUT_OF_BOUNDS|";
            continue;
        }
        if (!in_safe_zone(s.cur, ny, nx))
        {
            log_ss << ",OUT_OF_SAFE_ZONE|";
            continue;
        }
        if (M.blocked(ny, nx))
        {
            log_ss << ",BLOCKED|";
            continue;
        }
        if (M.is_danger(ny, nx) && me.shield_time == 0)
        {
            log_ss << ",DANGEROUS_NO_SHIELD|";
            continue;
        }

        // 计算可达性得�?
        BFSOut tempG = bfs_grid(M, s, ny, nx);
        int reachScore = 0;
        for (int y = 0; y < H; ++y)
            for (int x = 0; x < W; ++x)
                if (tempG.dist[y][x] < INF_DIST)
                    reachScore++;

        int base_reach = reachScore;

        // ==================== 新增：路径偏好评分?====================
        // 应用开阔度奖励（生存模式中这更重要�?
        double openness_score = calculate_openness(M, ny, nx);
        reachScore = (int)(reachScore * (1.0 + openness_score * 0.5));

        int penalty = 0;
        // 检查并惩罚危险路径
        if (is_dead_end(M, ny, nx))
        {
            penalty += 500; // 严重惩罚死路
            reachScore -= 500;
        }
        if (is_narrow_path(M, ny, nx))
        {
            penalty += 200; // 惩罚狭窄路径
            reachScore -= 200;
        }
        if (near_safe_zone_boundary(ny, nx))
        {
            penalty += 300; // 惩罚安全区边�?
            reachScore -= 300;
        }
        // ============================================================

        log_ss << ",base_reach=" << base_reach << ",openness=" << openness_score
               << ",penalty=" << penalty << ",final_score=" << reachScore;

        if (reachScore > bestReach)
        {
            bestReach = reachScore;
            bestDir = k;
            log_ss << ",NEW_BEST";
        }
        log_ss << "|";
    }

    if (bestDir != -1)
    {
        log_ss << "SURVIVAL_MOVE:DIR" << bestDir << "|";
        return ACT[bestDir];
    }

    // 尝试开�?
    if (me.shield_time == 0 && can_open_shield(me))
    {
        log_ss << "SURVIVAL_SHIELD|";
        return 4;
    }

    // 绝望移动（添加验证）
    int opposite_dir_desperate = (me.dir + 2) % 4;
    for (int k = 0; k < 4; ++k)
    {
        int ny = sy + DY[k], nx = sx + DX[k];

        // 防止掉头
        if (k == opposite_dir_desperate)
            continue;

        if (!in_bounds(ny, nx) || !in_safe_zone(s.cur, ny, nx) || M.blocked(ny, nx))
            continue;
        log_ss << "DESPERATE_DIR" << k << "|";
        return ACT[k];
    }

    log_ss << "FORCED_FALLBACK|";
    return 0;
}

// 检查第一步移动是否与敌人冲突
bool first_step_conflict(const State &s, int sy, int sx, int dir, bool opp_next[H][W])
{
    // 计算移动后的位置
    int ny = sy + DY[dir], nx = sx + DX[dir];
    // 检查是否与敌人的预测位置冲�?
    return opp_next[ny][nx];
}

// 安全地图内移动策略 - 当没有候选目标时优先考虑在安全区域内移动
// 增强功能：接近边界时优先移动到食物密度高的区域
int safe_map_movement(const State &s, int sy, int sx, stringstream &log_ss, const GridMask &M)
{
    log_ss << "SAFE_MAP_MOVEMENT:|";

    const auto &me = s.self();
    int opposite_dir = (me.dir + 2) % 4; // 防止掉头
    int best_dir = -1;
    double best_score = -1000.0;

    // 检测是否接近边界
    int boundary_distance = min({sy, sx, H - 1 - sy, W - 1 - sx});
    int safe_zone_boundary_dist = min({sy - s.cur.y_min, sx - s.cur.x_min,
                                       s.cur.y_max - sy, s.cur.x_max - sx});
    bool near_boundary = (boundary_distance <= 5) || (safe_zone_boundary_dist <= 3);

    if (near_boundary)
    {
        log_ss << "NEAR_BOUNDARY:map_dist=" << boundary_distance
               << ",safe_dist=" << safe_zone_boundary_dist << "|";
    }

    // 评估所有可能的移动方向
    for (int k = 0; k < 4; ++k)
    {
        if (k == opposite_dir)
            continue; // 不允许掉�?

        int ny = sy + DY[k], nx = sx + DX[k];

        // 基本安全检查：必须在地图边界内
        if (!in_bounds(ny, nx))
        {
            log_ss << "DIR" << k << ":OUT_OF_BOUNDS|";
            continue;
        }

        // 必须在安全区域内
        if (!in_safe_zone(s.cur, ny, nx))
        {
            log_ss << "DIR" << k << ":UNSAFE_ZONE|";
            continue;
        }
        // 不能撞墙或障碍物
        if (M.blocked(ny, nx))
        {
            log_ss << "DIR" << k << ":BLOCKED|";
            continue;
        }

        // 计算安全评分
        double safety_score = 100.0;

        // 检查蛇身碰撞（无护盾时间?
        if (M.is_snake(ny, nx) && me.shield_time == 0)
        {
            log_ss << "DIR" << k << ":SNAKE_COLLISION|";
            continue;
        }

        // 检查危险区域（无护盾时间?
        if (M.is_danger(ny, nx) && me.shield_time == 0)
        {
            log_ss << "DIR" << k << ":DANGER_ZONE|";
            safety_score *= 0.3; // 大幅降低危险区域的评分?
        }

        // 计算食物密度奖励
        double food_density_bonus = calculate_food_density_bonus(s, ny, nx);

        // 接近边界时的特殊策略
        if (near_boundary)
        {
            // 接近边界时，食物密度成为主要考虑因素
            safety_score *= food_density_bonus * 2.0; // 双倍食物密度奖励

            // 计算该方向是否远离边界
            int new_boundary_dist = min({ny, nx, H - 1 - ny, W - 1 - nx});
            int new_safe_boundary_dist = min({ny - s.cur.y_min, nx - s.cur.x_min,
                                              s.cur.y_max - ny, s.cur.x_max - nx});

            // 如果移动后远离边界，给予额外奖励
            if (new_boundary_dist > boundary_distance)
            {
                safety_score *= 1.3; // 远离地图边界奖励
            }
            if (new_safe_boundary_dist > safe_zone_boundary_dist)
            {
                safety_score *= 1.2; // 远离安全区边界奖励
            }

            // 检查该方向的食物可达性
            int reachable_food_count = 0;
            int high_value_food_count = 0;

            for (const auto &item : s.items)
            {
                if (item.type >= 1 || item.type == -1) // 普通食物或成长食物
                {
                    int dist_to_food = manhattan(ny, nx, item.pos.y, item.pos.x);
                    if (dist_to_food <= 8) // 8步内可达的食物
                    {
                        reachable_food_count++;
                        if (item.value >= 10) // 高价值食物
                        {
                            high_value_food_count++;
                        }
                    }
                }
            }

            // 根据可达食物数量和质量调整评分
            double food_accessibility_bonus = 1.0 + (reachable_food_count * 0.1) + (high_value_food_count * 0.2);
            safety_score *= food_accessibility_bonus;

            log_ss << "DIR" << k << ":BOUNDARY_MODE:food_bonus=" << (int)(food_density_bonus * 100)
                   << ",reachable_food=" << reachable_food_count
                   << ",high_value=" << high_value_food_count << "|";
        }
        else
        {
            // 非边界时的常规策略
            // 计算距离地图中心的距离，优先向地图中心移动
            int center_y = H / 2, center_x = W / 2;
            double dist_to_center = sqrt((ny - center_y) * (ny - center_y) + (nx - center_x) * (nx - center_x));
            double center_bonus = 1.0 + (1.0 / (1.0 + dist_to_center * 0.1)); // 距离中心越近奖励越高
            safety_score *= center_bonus;

            // 应用食物密度奖励（常规强度）
            safety_score *= food_density_bonus;
        }

        // 计算周围空间的开阔度
        int open_spaces = 0;
        for (int d = 0; d < 4; ++d)
        {
            int adj_y = ny + DY[d], adj_x = nx + DX[d];
            if (in_bounds(adj_y, adj_x) && !M.blocked(adj_y, adj_x) && in_safe_zone(s.cur, adj_y, adj_x))
                open_spaces++;
        }
        double openness_bonus = 1.0 + open_spaces * 0.2; // 开阔度奖励
        safety_score *= openness_bonus;

        // 避免地图边缘（但在接近边界时减轻惩罚）
        int edge_distance = min({ny, nx, H - 1 - ny, W - 1 - nx});
        if (edge_distance < 3)
        {
            double edge_penalty = near_boundary ? (0.8 + edge_distance * 0.05) : (0.5 + edge_distance * 0.1);
            safety_score *= edge_penalty; // 接近边界时减轻边缘惩罚
        }

        log_ss << "DIR" << k << ":SCORE=" << (int)(safety_score * 100) << "|";

        if (safety_score > best_score)
        {
            best_score = safety_score;
            best_dir = k;
        }
    }

    // 如果找到了安全的移动方向
    if (best_dir != -1)
    {
        log_ss << "SAFE_MOVE:DIR" << best_dir << ",SCORE=" << (int)(best_score * 100) << "|";
        return ACT[best_dir];
    }

    // 如果没有找到完全安全的方向，尝试开护盾
    if (me.shield_time == 0 && can_open_shield(me))
    {
        log_ss << "EMERGENCY_SHIELD|";
        return 4;
    }

    // 最后的备选方案：选择任何可行的方向（即使有风险）
    for (int k = 0; k < 4; ++k)
    {
        if (k == opposite_dir)
            continue;

        int ny = sy + DY[k], nx = sx + DX[k];

        if (in_bounds(ny, nx) && in_safe_zone(s.cur, ny, nx) && !M.blocked(ny, nx))
        {
            log_ss << "EMERGENCY_MOVE:DIR" << k << "|";
            return ACT[k];
        }
    }

    log_ss << "NO_SAFE_OPTIONS|";
    return 0; // 默认向左移动
}

// ==================== 主要决策函数 ====================
// 定义选择结构�?
struct Choice
{
    int action; // 动作编号�?-3为移动方向，4为开护盾
};

// 核心决策函数 - 蛇AI的大脑，负责分析当前状态并做出最优决�?
Choice decide(const State &s)
{
    stringstream log_ss;                    // 日志记录流，用于调试和分数?
    const auto &me = s.self();              // 获取我方蛇的引用
    int sy = me.head().y, sx = me.head().x; // 当前蛇头位置

    // === 调试信息记录 ===
    // 记录当前回合和位置信息，便于回放分析
    log_ss << "TURN:" << (int)s.current_ticks << ",MY:" << sy << "," << sx
           << ",DIR:" << me.dir << ",LEN:" << me.length << "|";

    // 记录当前食物状态（显示�?个食物的详细信息）?
    log_ss << "FOODS:";
    for (int i = 0; i < s.items.size() && i < 5; i++)
    {
        log_ss << "(" << s.items[i].pos.y << "," << s.items[i].pos.x
               << ",v=" << s.items[i].value << ",life=" << s.items[i].lifetime << ")";
    }
    log_ss << "|";

    // === 第一优先级：紧急情况处理（统一入口�?===
    if (!in_safe_zone(s.cur, sy, sx))
    {
        // 为了评估阻挡/危险，构建一个轻量掩码（与常规模型一致）
        GridMask emergencyM;
        for (const auto &sn : s.snakes)
            if (sn.id != MYID)
            {
                for (const auto &p : sn.body)
                    emergencyM.snake(p.y, p.x);
                auto h = sn.head();
                for (int k = 0; k < 4; ++k)
                    emergencyM.danger(h.y + DY[k], h.x + DX[k]);
            }
        for (const auto &it : s.items)
            if (it.type == -2)
                emergencyM.trap(it.pos.y, it.pos.x);
        int act = emergency_handle_outside_safe(s, log_ss, emergencyM);
        str_info += log_ss.str();
        return {act};
    }

    // 构建掩码（完整实现）
    GridMask M;

    // 填充蛇身
    for (const auto &sn : s.snakes)
    {
        if (sn.id != MYID)
        {
            // 标记敌方蛇身体为阻挡
            for (const auto &p : sn.body)
            {
                M.snake(p.y, p.x);
            }
            // 标记敌方蛇头周围为危险?
            auto head = sn.head();
            for (int k = 0; k < 4; k++)
            {
                int ny = head.y + DY[k], nx = head.x + DX[k];
                M.danger(ny, nx);
            }
        }
    }

    // 填充陷阱
    for (const auto &item : s.items)
    {
        if (item.type == -2) // 陷阱
        {
            M.trap(item.pos.y, item.pos.x);
        }
    }

    // 填充宝箱（如果没有钥匙则阻挡�?
    for (const auto &chest : s.chests)
    {
        if (!me.has_key)
        {
            M.block(chest.pos.y, chest.pos.x);
        }
    }

    // 以当前蛇头为起点做一次全�?BFS
    BFSOut G = bfs_grid(M, s, sy, sx);
    Point head{sy, sx};

    // === 候选目标构建函数?===
    // Lambda函数：智能构建候选目标集合，包括食物、成长食物、钥匙、宝箱?
    // 参数 K：最多保留的候选目标数量（默认24个），用于控制计算复杂度
    auto build_candidates = [&](int K = 24)
    {
        vector<Cand> C; // 候选目标容�?
        C.reserve(64);  // 预留空间以提高性能，避免频繁内存分数?

        // === 第一步：竞争分析 ===
        // 分析所有目标位置与敌方蛇的竞争情况，判断哪些目标可以安全获胜?
        vector<CompetitionAnalysis> competitions = analyze_competition(s, M, G);
        map<pair<int, int>, CompetitionAnalysis> competition_map;

        // === 构建竞争信息映射�?===
        // 为每个目标位置保留最危险的竞争分析（最难获取的情况�?
        // 这样可以做出最保守的决策，避免冒险争夺
        for (const auto &comp : competitions)
        {
            pair<int, int> key = {comp.target.y, comp.target.x};
            // 保留最危险的竞争分析：
            // 1. 敌人距离更近的情�?
            // 2. 距离相同但优势更小的情况
            if (competition_map.find(key) == competition_map.end() ||
                comp.enemy_dist < competition_map[key].enemy_dist ||
                (comp.enemy_dist == competition_map[key].enemy_dist && comp.advantage < competition_map[key].advantage))
            {
                competition_map[key] = comp;
            }
        }

        // === 调试信息：记录特定位置的竞争分析 ===
        // 用于调试和分析特定食物的竞争情况
        log_ss << "COMPETITION_DEBUG:|";
        for (const auto &comp : competitions)
        {
            // 只记录特定坐标的详细竞争信息（避免日志过多）
            if (comp.target.y == 11 && comp.target.x == 34)
            {
                log_ss << "FOOD(11,34):enemy" << comp.enemy_id
                       << ",my_d=" << comp.my_dist
                       << ",enemy_d=" << comp.enemy_dist
                       << ",i_win=" << (comp.i_win_tie ? "YES" : "NO")
                       << ",adv=" << (int)(comp.advantage * 100) << "|";
            }
        }

        // === 第二步：遍历所有游戏道具并评估其价值?===
        // 包括食物、成长食物、钥匙、宝箱等所有可收集物品
        for (const auto &it : s.items)
        {
            // === 基础过滤条件 ===
            // 1. 位置必须在游戏边界内
            // 2. 位置必须在当前安全区内（避免在危险区域冒险）
            if (!in_bounds(it.pos.y, it.pos.x) || !in_safe_zone(s.cur, it.pos.y, it.pos.x))
                continue;

            // === 可达性检查?===
            // 使用BFS预计算的距离信息，判断目标是否可用?
            int d = G.dist[it.pos.y][it.pos.x];
            if (d >= INF_DIST)
                continue; // 不可达的目标直接跳过

            // === 食物过期检查?===
            // 应用TTL软衰减而不是硬截断
            if (it.lifetime != -1) // -1表示永久有效的物�?
            {
                // 注意：如果食物到达时生命值很低，不会在这里跳�?
                // 而是通过TTL_SOFT_DECAY机制来降低其优先�?
            }

            // === 安全区收缩风险评分?===
            // 检查食物到达时和吃完后是否会在安全区收缩中受到影响
            int arrival_time = s.current_ticks + d; // 预计到达时间

            // 检查?：食物是否会在到达时已经在安全区外（由于收缩时?
            if (s.next_tick != -1 && arrival_time >= s.next_tick)
            {
                if (!in_safe_zone(s.next, it.pos.y, it.pos.x))
                {
                    // 食物将在安全区收缩后位于危险区域，直接跳?
                    if (C.size() < 3) // 只记录前几个以避免日志过?
                    {
                        log_ss << "SKIP_UNSAFE_ARRIVAL:(" << it.pos.y << "," << it.pos.x
                               << ")arrival_t=" << arrival_time << ",shrink_t=" << s.next_tick << "|";
                    }
                    continue;
                }
            }

            // 检查?：蛇吃完食物后是否能安全停留在该位置
            // 考虑吃食物需要额外?回合的时间?
            int post_eating_time = arrival_time + 1;
            if (s.next_tick != -1 && post_eating_time >= s.next_tick)
            {
                if (!in_safe_zone(s.next, it.pos.y, it.pos.x))
                {
                    // 吃完食物后该位置将不安全，应用惩罚而不是完全跳�?
                    // 这样可以在紧急情况下仍然选择该食物，但优先级降低
                    // 惩罚程度根据时间紧迫性调整?
                    // （惩罚逻辑将在后面的路径危险度惩罚部分处理�?
                    if (C.size() < 3)
                    {
                        log_ss << "POST_EAT_UNSAFE:(" << it.pos.y << "," << it.pos.x
                               << ")post_eat_t=" << post_eating_time << ",shrink_t=" << s.next_tick << "|";
                    }
                }
            }

            // === 竞争风险评估 ===
            // 检查是否应该避免这个目标，避免与敌方蛇发生致命冲突
            bool should_avoid = false;    // 是否完全避免这个目标
            int collision_risk_level = 0; // 碰撞风险等级 (0=无风�? 1=�? 2=�? 3=�?            // 只对普通食物和成长食物进行竞争风险评估
            // 钥匙和宝箱由于其特殊性，会在后面单独处理
            if (it.type >= 1 || it.type == -1) // 普通食物?type>=1)或成长食物?type=-1)
            {
                // 查找此目标的竞争分析信息
                pair<int, int> target_key = {it.pos.y, it.pos.x};
                auto comp_it = competition_map.find(target_key);
                if (comp_it != competition_map.end())
                {
                    const CompetitionAnalysis &comp = comp_it->second;

                    // 敌人明显更近的情况
                    if (comp.enemy_dist < comp.my_dist - 1)
                    {
                        collision_risk_level = 3; // 高风险
                        should_avoid = true;      // 完全避免
                    }
                    // 距离相等但根据游戏规则我们会输的情况
                    else if (comp.enemy_dist == comp.my_dist && !comp.i_win_tie)
                    {
                        collision_risk_level = 2; // 中风�?
                        // 不直接避免，但会大幅降低评分
                    }
                }
            }

            // 如果判断应该避免这个目标，直接跳�?
            if (should_avoid)
            {
                // 可以在这里记录避免的目标用于调试分析
                continue;
            }

            // === 获取目标的基础价值 ===
            int v = it.value; // 道具的原始分数价值

            // === 计算基础评分 ===
            // 基础公式化?价值?* 生命值因素? / (距离 + 偏移动?
            // DISTANCE_OFFSET防止除零，并平衡近距离目标的权重
            double base_score = (v) / (d + DISTANCE_OFFSET);

            // === 应用碰撞风险惩罚 ===
            // 根据前面计算的collision_risk_level调整评分
            double risk_penalty = 1.0;
            switch (collision_risk_level)
            {
            case 1:
                risk_penalty = 0.8; // 低风险：轻微降分(20%惩罚)
                break;
            case 2:
                risk_penalty = 0.6; // 中风险：明显降分(40%惩罚)
                break;
            case 3:
                risk_penalty = 0.3; // 高风险：大幅降分(70%惩罚)
                break;
            default:
                risk_penalty = 1.0; // 无风险?
                break;
            }
            base_score *= risk_penalty;

            // === 第三步：路径选择偏好优化 ===
            // 这部分对评分进行多维度的智能调整，提升决策质?

            // 保存原始评分用于调试对比
            double original_score = base_score;

            // === 开阔区域奖励?===
            // 优先选择开阔区域的目标，避免进入狭窄危险的区域
            base_score = apply_openness_bonus(M, it.pos.y, it.pos.x, base_score);

            // === 食物密度奖励 ===
            // 选择食物密集区域，提高觅食效率
            double food_density_bonus = calculate_food_density_bonus(s, it.pos.y, it.pos.x);
            base_score *= food_density_bonus;

            // === 路径危险度惩罚 ===
            // 计算并扣除路径上的各种危险因素分数，包括安全区收缩风险
            int danger_penalty = calculate_path_danger_penalty(M, it.pos.y, it.pos.x, v, d);
            base_score -= danger_penalty;

            // === 分数安全检查 ===
            // 确保正价值物品的分数不会因为惩罚而变成负数
            if (v > 0 && base_score < 0)
            {
                base_score = 1; // 给予最小正分，避免完全忽略有价值的目标
            }

            // === 调试信息记录 ===
            // 记录前几个候选目标的详细评分调整过程（避免日志过多）
            if (C.size() < 3)
            {
                double openness = calculate_openness(M, it.pos.y, it.pos.x);
                log_ss << "PATH_PREF:(" << it.pos.y << "," << it.pos.x << ")"
                       << "open=" << (int)(openness * 100)
                       << ",fd_bonus=" << (int)(food_density_bonus * 100)
                       << ",penalty=" << danger_penalty
                       << ",score:" << (int)(original_score * 100) << "->" << (int)(base_score * 100) << "|";
            }

            // === 第四步：应用竞争优势调整 ===
            // 根据与敌方的竞争分析结果，进一步调整目标的最终评分?
            pair<int, int> target_key = {it.pos.y, it.pos.x};
            auto comp_it = competition_map.find(target_key);
            double final_score = base_score;

            if (comp_it != competition_map.end())
            {
                const CompetitionAnalysis &comp = comp_it->second;

                if (comp.advantage > 0) // 我方有优化?
                {
                    // 小幅提升评分，优势越大
                    final_score *= (1.0 + comp.advantage * 0.2);
                    if (comp.advantage >= 3.0) // 巨大优势
                    {
                        final_score *= 1.5; // 额外50%奖励
                    }
                }
                else if (comp.advantage < -1.0) // 我方明显劣势
                {
                    // 大幅降低评分，劣势越大降低越明显
                    final_score *= (1.0 + comp.advantage * 0.3);
                    if (comp.advantage <= -3.0) // 巨大劣势
                    {
                        final_score *= 0.3; // 仅保留30%分数
                    }
                }
                else // 势均力敌的情况
                {
                    // 根据平局规则调整：我方能赢平局则略微提升，否则略微降低
                    final_score *= (comp.i_win_tie ? 1.1 : 0.9);
                }
            }
            // === 第五步：钥匙-宝箱智能处理逻辑 ===

            // === 钥匙处理逻辑 ===
            if (it.type == -3) // 钥匙类型
            {
                if (me.has_key)
                {
                    // 已经有钥匙了，跳过其他钥匙
                    continue;
                }
                else
                {
                    // 没有钥匙时，评估钥匙的价值应该考虑附近宝箱的收缩
                    double key_bonus = 1.0; // 钥匙价值倍数

                    // 查找钥匙附近的所有宝箱?
                    for (const auto &chest : s.chests)
                    {
                        int chest_dist_from_key = manhattan(it.pos.y, it.pos.x, chest.pos.y, chest.pos.x);
                        if (chest_dist_from_key <= 20) // 钥匙20步范围内有宝箱?
                        {
                            // 计算钥匙价值：宝箱价值密度 = 宝箱分数 / 总路径长度
                            int total_dist = d + chest_dist_from_key;
                            double chest_value_density = (double)chest.score / (total_dist + 1);
                            // 根据最佳宝箱价值密度提升钥匙评分
                            key_bonus = max(key_bonus, chest_value_density / 10.0);
                        }
                    }
                    final_score *= key_bonus;
                    log_ss << "KEY_BONUS:" << (int)(key_bonus * 100) << "|";
                }
            }

            // === 宝箱处理逻辑 ===
            if (it.type == -5) // 宝箱类型
            {
                if (!me.has_key)
                {
                    // 没有钥匙时跳过宝箱，避免浪费时间甚至导致死亡
                    continue;
                }
                else
                {
                    // 有钥匙时，宝箱是高优先级目标
                    final_score *= 1.5; // 提升50%优先级
                    log_ss << "CHEST_PRIORITY_BOOST|";
                }
            }

            // === 应用TTL软衰减而不是硬截断 ===
            if (!ttl_soft_decay(final_score, d, it.lifetime))
            {
                // 分数变得微不足道 -> 跳过以减少噪音
                continue;
            }

            // === 将评估后的候选目标加入列表 ===
            C.push_back({it.pos, v, d, final_score, it.lifetime});
        }
        // === 第六步：候选目标排序和筛选 ===

        // 如果没有找到任何有效的候选目标，直接返回空列表
        if (C.empty())
            return C;

        // 按照最终评分进行排序：分数高的优先，分数相同时距离近的优先
        sort(C.begin(), C.end(), [](const Cand &a, const Cand &b)
             {
                // 首要条件：按分数降序排列
                if (a.score != b.score) 
                    return a.score > b.score;
                // 次要条件：分数相同时按距离升序排�?
                return a.d < b.d; });

        // 限制候选目标数量，避免后续计算过于复杂
        if ((int)C.size() > K)
            C.resize(K);

        return C; // 返回排序后的候选目标列表
    };

    auto C = build_candidates();
    log_ss << "CANDIDATES_COUNT:" << C.size() << "|";

    // 添加调试信息：显示前几个候选目标
    for (int i = 0; i < min(5, (int)C.size()); i++)
    {
        log_ss << "CAND" << i << ":(" << C[i].p.y << "," << C[i].p.x
               << ")d=" << C[i].d << ",v=" << C[i].val << ",s=" << (int)(C[i].score * 100) << "|";
    }

    // 如果没有候选目标，优先考虑在地图内安全移动
    if (C.empty())
    {
        int choice = safe_map_movement(s, sy, sx, log_ss, M);
        str_info += log_ss.str();
        return {choice};
    }

    // 构建并改进路径
    Route R;
    build_route_greedy(M, s, C, head, R);
    improve_route_ls(M, s, head, R);

    log_ss << "ROUTE_ITEMS:" << R.seq.size()
           << ",VAL:" << R.totalVal
           << ",TIME:" << R.finishT << "|";

    // 如果路径为空，使用后备策略
    if (R.seq.empty())
    {
        log_ss << "ENTER_SURVIVAL_MODE:[ROUTE_EMPTY]|";
        int choice = survival_strategy(s, sy, sx, log_ss, M);
        str_info += log_ss.str();
        return {choice};
    }

    // 🔧 改进：考虑多步路径规划，而不只是第一个目标
    Point goal;
    if (R.seq.size() >= 2)
    {
        // 检查是否值得为了更远的高价值目标而跳过近处的低价值目标
        Point near_goal = R.seq[0];
        Point far_goal = R.seq[1];

        // 计算近目标和远目标的价值密度
        int near_dist = G.dist[near_goal.y][near_goal.x];
        int far_dist = G.dist[far_goal.y][far_goal.x];

        // 找到对应的候选项来获取价值信息
        int near_value = 0, far_value = 0;
        for (const auto &cand : C)
        {
            if (cand.p.y == near_goal.y && cand.p.x == near_goal.x)
            {
                near_value = cand.val;
            }
            if (cand.p.y == far_goal.y && cand.p.x == far_goal.x)
            {
                far_value = cand.val;
            }
        }

        // 如果远目标的价值密度明显更高，且距离差不太大，选择远目标
        double near_density = (double)near_value / (near_dist + 1);
        double far_density = (double)far_value / (far_dist + 1);

        if (far_density > near_density * 1.5 && far_dist <= near_dist + 5)
        {
            goal = far_goal;
            log_ss << "CHOOSE_FAR_TARGET:near_d=" << near_density << ",far_d=" << far_density << "|";
        }
        else
        {
            goal = near_goal;
            log_ss << "CHOOSE_NEAR_TARGET:near_d=" << near_density << ",far_d=" << far_density << "|";
        }
    }
    else
    {
        // 只有一个目标或路径规划失败，使用第一个目标
        goal = R.seq.front();
        log_ss << "SINGLE_TARGET|";
    }

    // 最终避免检查：如果目标是食物且敌人更有优势，寻找替代目标
    bool target_is_food = false;
    for (const auto &item : s.items)
    {
        if (item.pos.y == goal.y && item.pos.x == goal.x &&
            (item.type >= 1 || item.type == -1)) // 普通食物或成长食物
        {
            target_is_food = true;
            break;
        }
    }

    if (target_is_food)
    {
        // 检查是否有敌人也在争夺这个食物
        bool enemy_contest = false;
        int my_dist = G.dist[goal.y][goal.x];

        for (const auto &enemy : s.snakes)
        {
            if (enemy.id == MYID)
                continue;

            BFSOut enemy_G = bfs_grid(M, s, enemy.head().y, enemy.head().x);
            int enemy_dist = enemy_G.dist[goal.y][goal.x];

            if (enemy_dist < INF_DIST)
            {
                // 如果敌人距离更近，或距离相等但敌人在平局中获胜?
                if (enemy_dist < my_dist ||
                    (enemy_dist == my_dist && !determine_tie_winner(me, enemy, goal, my_dist, enemy_dist)))
                {
                    enemy_contest = true;
                    break;
                }
            }
        }

        if (enemy_contest)
        {
            log_ss << "AVOIDING_CONTESTED_FOOD:FINDING_SAFE_ALTERNATIVE|";

            // 智能选择替代目标：寻找敌人无法快速到达的区域
            Point best_alternative = {-1, -1};
            double best_safety_score = -1.0;

            // 遍历所有候选目标，寻找最安全的替代方向?
            for (const auto &candidate : C)
            {
                // 跳过当前争夺的目?
                if (candidate.p.y == goal.y && candidate.p.x == goal.x)
                    continue;

                int my_dist_to_alt = G.dist[candidate.p.y][candidate.p.x];
                if (my_dist_to_alt >= INF_DIST)
                    continue;

                // 计算安全评分：考虑敌人到此目标的最小距离?
                int min_enemy_dist = INT_MAX;
                bool any_enemy_closer = false;

                for (const auto &enemy : s.snakes)
                {
                    if (enemy.id == MYID)
                        continue;

                    BFSOut enemy_G_alt = bfs_grid(M, s, enemy.head().y, enemy.head().x);
                    int enemy_dist_to_alt = enemy_G_alt.dist[candidate.p.y][candidate.p.x];

                    if (enemy_dist_to_alt < INF_DIST)
                    {
                        min_enemy_dist = min(min_enemy_dist, enemy_dist_to_alt);
                        if (enemy_dist_to_alt <= my_dist_to_alt)
                        {
                            any_enemy_closer = true;
                        }
                    }
                }

                // 如果有敌人也能更快到达此目标，跳?
                if (any_enemy_closer)
                    continue;

                // 计算安全评分：敌人距离越远越好，我的距离越近越好
                double safety_score = 0.0;
                if (min_enemy_dist != INT_MAX)
                {
                    // 距离优势 + 价值权重
                    double distance_advantage = (double)(min_enemy_dist - my_dist_to_alt);
                    safety_score = distance_advantage * 2.0 + candidate.score * 0.1;

                    // 奖励距离差距大的目标
                    if (distance_advantage >= 3.0)
                        safety_score *= 1.5;
                }
                else
                {
                    // 敌人完全无法到达的目标给予最高优先级
                    safety_score = 1000.0 + candidate.score * 0.1;
                }

                if (safety_score > best_safety_score)
                {
                    best_safety_score = safety_score;
                    best_alternative = candidate.p;
                }
            }

            // 如果找到了安全的替代目标
            if (best_alternative.y != -1)
            {
                goal = best_alternative;
                log_ss << "FOUND_SAFE_ALTERNATIVE:(" << goal.y << "," << goal.x
                       << ")safety=" << (int)(best_safety_score * 100) << "|";
            }
            else
            {
                // 如果所有备选目标都不安全，尝试序列中的下一目标
                if (R.seq.size() > 1)
                {
                    goal = R.seq[1];
                    log_ss << "USING_SECONDARY_TARGET_FALLBACK|";
                }
                else
                {
                    // 最后选择：寻找敌人密度最低的安全区域移动
                    log_ss << "NO_SAFE_FOOD:FINDING_LOW_ENEMY_DENSITY_AREA|";

                    int best_dir = -1;
                    double best_area_score = -1.0;
                    int opposite_dir_area = (me.dir + 2) % 4;

                    for (int k = 0; k < 4; k++)
                    {
                        if (k == opposite_dir_area)
                            continue;

                        int ny = head.y + DY[k], nx = head.x + DX[k];
                        if (!in_bounds(ny, nx) || !in_safe_zone(s.cur, ny, nx) || M.blocked(ny, nx))
                            continue;
                        if (M.is_danger(ny, nx) && me.shield_time == 0)
                            continue;

                        // 计算此方向的敌人威胁密度
                        double area_score = 0.0;
                        int enemy_threats = 0;

                        // 检查?x3区域内的敌人威胁
                        for (int dy = -1; dy <= 1; dy++)
                        {
                            for (int dx = -1; dx <= 1; dx++)
                            {
                                int check_y = ny + dy, check_x = nx + dx;
                                if (!in_bounds(check_y, check_x))
                                    continue;

                                for (const auto &enemy : s.snakes)
                                {
                                    if (enemy.id == MYID)
                                        continue;

                                    int dist_to_enemy_head = manhattan(check_y, check_x,
                                                                       enemy.head().y, enemy.head().x);
                                    if (dist_to_enemy_head <= 2)
                                    {
                                        enemy_threats++;
                                    }
                                }
                            }
                        }

                        // 敌人威胁越少，评分越�?
                        area_score = 100.0 - enemy_threats * 10.0;

                        // 计算该方向的可达性奖励?
                        BFSOut temp_G = bfs_grid(M, s, ny, nx);
                        int reachable_cells = 0;
                        for (int y = 0; y < H; y++)
                        {
                            for (int x = 0; x < W; x++)
                            {
                                if (temp_G.dist[y][x] < 10) // 10步内可达
                                    reachable_cells++;
                            }
                        }
                        area_score += reachable_cells * 0.5;

                        // ==================== 新增：路径偏好评分?====================
                        // 应用开阔度奖励
                        double openness = calculate_openness(M, ny, nx);
                        area_score *= (1.0 + openness * 0.3); // 30%的开阔度奖励

                        // 应用危险路径惩罚
                        int danger_penalty = calculate_path_danger_penalty(M, ny, nx, 50);
                        area_score -= danger_penalty * 0.5; // 减轻惩罚以避免过度影�?
                        // ============================================================

                        if (area_score > best_area_score)
                        {
                            best_area_score = area_score;
                            best_dir = k;
                        }
                    }

                    if (best_dir != -1)
                    {
                        log_ss << "SAFE_AREA_MOVE:DIR" << best_dir
                               << ",score=" << (int)best_area_score << "|";
                        str_info += log_ss.str();
                        return {ACT[best_dir]};
                    }

                    log_ss << "ENTER_SURVIVAL_MODE:[NO_SAFE_AREA]|";
                    int choice = survival_strategy(s, sy, sx, log_ss, M);
                    str_info += log_ss.str();
                    return {choice};
                }
            }
        }
    }

    auto G2 = bfs_grid(M, s, head.y, head.x);
    log_ss << "BFS_RESULT:to_goal_dist=" << G2.dist[goal.y][goal.x] << "|";
    if (G2.dist[goal.y][goal.x] >= INF_DIST)
    {
        log_ss << "ENTER_SURVIVAL_MODE:[TARGET_UNREACHABLE_G2_CHECK]goal=(" << goal.y << "," << goal.x << ")|";
        int choice = survival_strategy(s, sy, sx, log_ss, M);
        str_info += log_ss.str();
        return {choice};
    }

    // 使用A*算法计算第一步（替换原来的BFS路径重构）
    int dir = astar_first_step(M, s, {head.y, head.x}, goal);

    log_ss << "ASTAR_RESULT:head=(" << head.y << "," << head.x << "),goal=(" << goal.y << "," << goal.x << "),dir=" << dir << "|";

    if (dir == -1)
    {
        log_ss << "ENTER_SURVIVAL_MODE:[ASTAR_UNREACHABLE]goal=(" << goal.y << "," << goal.x << ")|";
        int choice = survival_strategy(s, sy, sx, log_ss, M);
        str_info += log_ss.str();
        return {choice};
    }

    // prevent 180° reverse
    int opposite_dir = (s.self().dir + 2) % 4;
    if (dir == opposite_dir)
    {
        log_ss << "ENTER_SURVIVAL_MODE:[WOULD_REVERSE]dir=" << dir << ",opposite=" << opposite_dir << "|";
        int choice = survival_strategy(s, sy, sx, log_ss, M);
        str_info += log_ss.str();
        return {choice};
    }

    // 新增：针对缩圈的最后一英里安全防护
    int final_dir = enforce_future_safezone_or_fallback(s, M, sy, sx, dir, log_ss);

    // （保留原有的漂亮日志记录）
    static const char *DSTR[4] = {"LEFT", "UP", "RIGHT", "DOWN"};
    log_ss << "MULTI_TARGET_MOVE:" << DSTR[final_dir] << ",a:" << ACT[final_dir] << "|";
    str_info += log_ss.str();
    return {ACT[final_dir]};
}

// ==================== 主程序入口 ====================

/**
 * 主函数?
 *
 * 程序流程序?
 * 1. 读取游戏状态?
 * 2. 执行决策算法
 * 3. 输出动作选择
 *
 * 这是一个高效的贪吃蛇AI，专注于用?
 * - 快速BFS路径规划
 * - 智能的目标评分?
 * - 安全性优先的移动策略
 * - �?.7秒时间限制内完成所有计算?
 */
int main()
{
    // #ifndef ONLINE_JUDGE
    //     freopen("D:/su25-program/snake/input.in", "r", stdin); // debug only
    // #endif
    // ios::sync_with_stdio(false);
    cin.tie(nullptr);

    read_state(global_state);
    auto choice = decide(global_state);
    cout << choice.action << "\n";
    // cout << str_info << "\n";
    return 0;
}
