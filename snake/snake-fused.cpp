#include <bits/stdc++.h>
using namespace std;

// important: rule:与自己的蛇不会发生碰撞

// ==================== 游戏常量定义 ====================
static constexpr int W = 40;            // 地图宽度 (x坐标最大值)
static constexpr int H = 30;            // 地图高度 (y坐标最大值)
static constexpr int MYID = 2024201540; // TODO: 设置为你的学号

// ==================== 护盾和移动代价常量 ====================
static constexpr int SNAKE_COST_WEIGHT = 10;     // 蛇身代价权重
static constexpr int SNAKE_COST_NO_SHIELD = 100; // 无护盾时穿过蛇身的高代价
static constexpr int SNAKE_COST_WITH_SHIELD = 0; // 有护盾时穿过蛇身的代价
static constexpr int SNAKE_COST_OPEN_SHIELD = 2; // 使用护盾来穿过蛇身的代价
static constexpr int SHIELD_COST_THRESHOLD = 20; // 使用护盾所需的最低分数
static constexpr int TRAP_STEP_COST = 30;        // 陷阱步骤惩罚代价，用于路径规划中软性避开陷阱

// ==================== 食物和物品价值常量 ====================
static constexpr int GROWTH_FOOD_VALUE = 5;      // 成长食物价值
static constexpr int TRAP_PENALTY = -10;         // 陷阱扣分（负值）
static constexpr int KEY_VALUE = 75;             // 钥匙价值
static constexpr int CHEST_VALUE = 100;          // 宝箱基础价值
static constexpr int NORMAL_FOOD_MULTIPLIER = 3; // 普通食物价值倍数（改为1以匹配规则1-5分）

// ==================== 评分和权重常量 ====================
static constexpr double SNAKE_SAFETY_PENALTY_RATE = 0.5; // 蛇身穿越安全惩罚率（每个蛇身格子的惩罚倍数）
static constexpr double CHEST_SCORE_MULTIPLIER = 2.0;    // 宝箱评分倍数
static constexpr double DEFAULT_CHEST_SCORE = 60.0;      // 默认宝箱分数（当宝箱分数<=0时使用）
static constexpr double DISTANCE_OFFSET = 1.0;           // 距离计算偏移量，避免除零
static constexpr int SCORE_DISPLAY_MULTIPLIER = 100;     // 分数显示时的放大倍数（用于日志输出）
static constexpr double LIFETIME_SOFT_DECAY = 0.85;      // 寿命衰减底数（每多1步到达，乘以此因子）
static constexpr double CONTEST_PENALTY = 0.4;           // 竞争目标惩罚系数（被对手同样或更快到达的目标评分乘以此值）
static constexpr double NEXTZONE_RISK_PENALTY = 0.35;    // 缩圈前无法抵达且目标不在下个安全区时的折扣
static constexpr double DEGREE_BONUS = 0.06;             // 局部自由度奖励系数（每个安全邻居给予的乘数奖励）

// ==================== 前瞻算法常量 ====================
static constexpr int LOOKAHEAD_DEPTH = 1; // 1=启用一步前瞻；0=关闭

// ==================== 风险评估常量 ====================
static constexpr int SAFE_ZONE_BOUNDARY_THRESHOLD = 2;   // 安全区边界危险邻近阈值
static constexpr int SAFE_ZONE_SHRINK_THRESHOLD = 4;     // 安全区收缩阈值
static constexpr int ENEMY_BODY_PROXIMITY_THRESHOLD = 2; // 敌蛇身体危险邻近阈值
static constexpr int TRAP_PROXIMITY_THRESHOLD = 2;       // 陷阱危险邻近阈值
static constexpr int NEAR_ENEMY_ADJ_PENALTY = 3;         // 邻近敌蛇身体的额外代价

// ==================== 路径选择偏好常量 ====================
static constexpr double OPEN_AREA_BONUS = 1.3;         // 开阔区域奖励倍数
static constexpr int DEAD_END_PENALTY = 200;           // 死路惩罚分数
static constexpr int NARROW_PATH_PENALTY = 100;        // 狭窄路径惩罚分数
static constexpr int SAFE_ZONE_BOUNDARY_PENALTY = 150; // 安全区边界惩罚分数
static constexpr int MIN_REWARD_THRESHOLD = 100;       // 最小奖励阈值，低于此值的危险路径将被重罚
static constexpr int OPENNESS_RADIUS = 3;              // 开阔度检测半径
static constexpr int MIN_ESCAPE_ROUTES = 2;            // 最小逃生路线数量

// ==================== 数据结构定义 ====================
// 与游戏引擎格式对齐的结构体

/**
 * 坐标点结构
 */
struct Point
{
    int y, x; // y: 行坐标, x: 列坐标
};

/**
 * 游戏物品结构
 * value含义：
 * - 正数: 普通食物的分值
 * - -1: 成长食物（使蛇身变长）
 * - -2: 陷阱（有害） //扣10分（规则中为扣10分）
 * - -3: 钥匙
 * - -5: 宝箱
 */
struct Item
{
    Point pos;    // 物品位置
    int type;     // 物品类型
    int value;    // 物品价值
    int lifetime; // 物品剩余生存时间 (-1表示永久)
};

/**
 * 蛇的完整状态
 */
struct Snake
{
    int id, length, score, dir;                        // ID、长度、分数、移动方向
    int shield_cd, shield_time;                        // 护盾冷却时间、护盾持续时间
    bool has_key = false;                              // 是否持有钥匙
    vector<Point> body;                                // 蛇身体坐标列表（头部在前）
    const Point &head() const { return body.front(); } // 获取蛇头位置
};

/**
 * 宝箱结构
 */
struct Chest
{
    Point pos; // 宝箱位置
    int score; // 宝箱分数
};

/**
 * 钥匙结构
 */
struct Key
{
    Point pos;          // 钥匙位置
    int holder_id;      // 持有者ID (-1表示无人持有)
    int remaining_time; // 钥匙剩余有效时间
};

/**
 * 安全区域边界
 */
struct Safe
{
    int x_min, y_min, x_max, y_max; // 安全区域的边界坐标
};

/**
 * 游戏状态结构
 * 包含完整的游戏状态信息
 */
struct State
{
    int current_ticks;                 // 当前回合数
    int remaining_ticks;               // 游戏剩余回合数
    vector<Item> items;                // 地图上的所有物品
    vector<Snake> snakes;              // 游戏中的所有蛇
    vector<Chest> chests;              // 地图上的所有宝箱
    vector<Key> keys;                  // 地图上的所有钥匙
    vector<Point> traps;               // 地图上的所有陷阱（从items中提取）
    Safe cur, next, fin;               // 当前、下次、最终安全区域
    int next_tick = -1, fin_tick = -1; // 安全区缩小的时间点
    int self_idx = -1;                 // 自己蛇在snakes数组中的索引

    // 获取自己蛇的便捷方法
    const Snake &self() const { return snakes[self_idx]; }
};

/**
 * 路径规划结构
 */
struct Route
{
    vector<Point> seq; // 目标序列（仅食物和成长食物）
    int totalVal = 0;  // 总价值
    int finishT = 0;   // 完成时间
};

/**
 * 时间线结构
 */
struct Timeline
{
    vector<int> arrive, leave; // 到达和离开时间，与seq对齐
    bool feasible = true;      // 是否可行
};

/**
 * 竞争分析结构
 */
struct CompetitionAnalysis
{
    Point target;
    int my_dist;
    int enemy_dist;
    int enemy_id;
    bool i_win_tie;
    double advantage;
};

/**
 * 候选目标结构
 */
struct Cand
{
    Point p;
    int val;
    int d;
    double score;
    int ttl;
};

// 平局胜负判定函数
bool determine_tie_winner(const Snake &me, const Snake &enemy, const Point &target, int my_dist, int enemy_dist)
{
    if (my_dist < enemy_dist)
        return true;
    if (my_dist > enemy_dist)
        return false;
    if (me.shield_time > 0 && enemy.shield_time == 0)
        return true;
    if (me.shield_time == 0 && enemy.shield_time > 0)
        return false;
    if (me.length > enemy.length)
        return true;
    if (me.length < enemy.length)
        return false;
    if (me.score > enemy.score)
        return true;
    if (me.score < enemy.score)
        return false;
    return me.id > enemy.id;
}

// 是否可以开启护盾
inline bool can_open_shield(const Snake &me)
{
    return me.shield_cd == 0 && me.score >= SHIELD_COST_THRESHOLD;
}

string str_info;
State global_state; // 全局游戏状态

// ==================== 输入输出：每回合读取游戏状态 ====================

/**
 * 从标准输入读取游戏状态（API格式）
 * 优化了输入性能
 */
static void read_state(State &s)
{

    // 读取剩余回合数，如果读取失败则退出
    if (!(cin >> s.remaining_ticks))
        exit(0);
    s.current_ticks = 256 - s.remaining_ticks;
    // ========== 读取物品信息 ==========
    int m;
    cin >> m;
    s.items.resize(m);
    for (int i = 0; i < m; i++)
    {
        cin >> s.items[i].pos.y >> s.items[i].pos.x >> s.items[i].type >> s.items[i].lifetime;
        // 注意：此时还没有读取蛇信息，所以不能使用 s.self()
        switch (s.items[i].type)
        {
        case -1: // 成长食物
        {
            int stage_factor = (s.current_ticks < 81 ? 12 : (s.current_ticks < 201 ? 8 : 4)); // 动态调整
            s.items[i].value = stage_factor;
        }
        break;
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

    // ========== 读取蛇信息 ==========
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
        cout << "|ERR:SNAKE_NOT_FOUND\n";
        exit(0);
    }

    // ========== 重新计算食物价值（现在可以使用 s.self()） ==========
    const auto &me = s.self();
    for (auto &item : s.items)
    {
        if (item.type >= 1) // 普通食物
        {
            int base_v = item.type;
            int growth_bonus = ((me.score + base_v) / 20 > me.score / 20) ? 10 : 0; // 动态增长奖励
            item.value = base_v + growth_bonus;
        }
    }

    // ========== 读取宝箱信息 ==========
    int nc;
    cin >> nc;
    s.chests.resize(nc);
    for (int i = 0; i < nc; i++)
        cin >> s.chests[i].pos.y >> s.chests[i].pos.x >> s.chests[i].score;

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

    // 可选的内存行
    cin >> str_info; // 取消注释以启用内存系统
    // TODO: 解析str_info以实现跨回合持久化（如上一目标）
}

// ==================== 辅助函数 ====================

/**
 * 检查坐标是否在地图边界内
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
 * 若坐标在当前安全区 z 内：
 *   1) 如果下次收缩不存在 (next_tick == -1) 或 距离收缩还有 > SAFE_ZONE_SHRINK_THRESHOLD 回合，返回 0
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

    // 不会在 SAFE_ZONE_SHRINK_THRESHOLD 回合内收缩
    if (next_tick == -1 || (next_tick - now_tick) > SAFE_ZONE_SHRINK_THRESHOLD)
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

/**
 * 网格掩码结构
 * 使用位掩码高效存储地图状态
 */
struct GridMask
{
    bitset<W> blocked_rows[H]; // 墙位置的位掩码
    bitset<W> snake_rows[H];   // 敌方蛇身体位置的位掩码
    bitset<W> danger_rows[H];  // 危险位置的位掩码
    bitset<W> trap_rows[H];    // 陷阱位置的位掩码
    /**
     * 标记位置为墙
     */
    inline void block(int y, int x)
    {
        if (in_bounds(y, x) && in_safe_zone(global_state.cur, y, x))
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
     * 标记位置为危险-蛇头周围
     */
    inline void danger(int y, int x)
    {
        if (in_bounds(y, x) && in_safe_zone(global_state.cur, y, x))
            danger_rows[y].set(x);
    }
    /**
     * 标记位置为陷阱
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
     * 检查位置是否是敌方蛇身体
     */
    inline bool is_snake(int y, int x) const
    {
        return (in_bounds(y, x) && in_safe_zone(global_state.cur, y, x)) ? snake_rows[y].test(x) : false;
    }

    /**
     * 检查位置是否危险
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

// ==================== 路径选择偏好函数 ====================

/**
 * 计算位置的开阔度分数 - 检查周围可自由移动的空间
 * @param M GridMask引用，用于检查障碍物
 * @param y, x 要检查的位置坐标
 * @return 开阔度分数（0-1之间）
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
                continue; // 跳过中心点

            int ny = y + dy, nx = x + dx;
            if (in_bounds(ny, nx) && in_safe_zone(global_state.cur, ny, nx))
            {
                total_spaces++;
                if (!M.blocked(ny, nx) && !M.is_danger(ny, nx))
                {
                    free_spaces++;
                }
            }
        }
    }

    return total_spaces > 0 ? (double)free_spaces / total_spaces : 0.0;
}

/**
 * 检查位置是否为死路 - 只有一个或零个逃生方向
 * @param M GridMask引用
 * @param y, x 要检查的位置坐标
 * @return true如果是死路
 */
bool is_dead_end(const GridMask &M, int y, int x)
{
    int escape_routes = 0;

    // 检查四个基本方向
    for (int k = 0; k < 4; k++)
    {
        int ny = y + DY[k], nx = x + DX[k];
        if (in_bounds(ny, nx) && in_safe_zone(global_state.cur, ny, nx) &&
            !M.blocked(ny, nx) && !M.is_danger(ny, nx))
        {
            escape_routes++;
        }
    }

    return escape_routes < MIN_ESCAPE_ROUTES;
}

/**
 * 检查路径是否过于狭窄 - 缺乏机动空间
 * @param M GridMask引用
 * @param y, x 要检查的位置坐标
 * @return true如果路径狭窄
 */
bool is_narrow_path(const GridMask &M, int y, int x)
{
    // 检查是否有足够的侧向空间
    int lateral_space = 0;

    // 检查左右两侧
    for (int dx = -2; dx <= 2; dx++)
    {
        if (dx == 0)
            continue;
        int nx = x + dx;
        if (in_bounds(y, nx) && in_safe_zone(global_state.cur, y, nx) &&
            !M.blocked(y, nx) && !M.is_danger(y, nx))
        {
            lateral_space++;
        }
    }

    // 检查上下两侧
    for (int dy = -2; dy <= 2; dy++)
    {
        if (dy == 0)
            continue;
        int ny = y + dy;
        if (in_bounds(ny, x) && in_safe_zone(global_state.cur, ny, x) &&
            !M.blocked(ny, x) && !M.is_danger(ny, x))
        {
            lateral_space++;
        }
    }

    return lateral_space < 3; // 如果侧向空间少于3个格子，认为是狭窄路径
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
 * 计算路径的综合危险度惩罚
 * @param M GridMask引用
 * @param y, x 位置坐标
 * @param expected_reward 预期奖励
 * @return 惩罚分数（正数表示惩罚）
 */
int calculate_path_danger_penalty(const GridMask &M, int y, int x, int expected_reward)
{
    int penalty = 0;

    // 死路检查
    if (is_dead_end(M, y, x))
    {
        penalty += DEAD_END_PENALTY;
        // 如果奖励很低，额外加重惩罚
        if (expected_reward < MIN_REWARD_THRESHOLD)
        {
            penalty += DEAD_END_PENALTY;
        }
    }

    // 狭窄路径检查
    if (is_narrow_path(M, y, x))
    {
        penalty += NARROW_PATH_PENALTY;
        if (expected_reward < MIN_REWARD_THRESHOLD)
        {
            penalty += NARROW_PATH_PENALTY / 2;
        }
    }

    // 安全区边界检查
    if (near_safe_zone_boundary(y, x))
    {
        penalty += SAFE_ZONE_BOUNDARY_PENALTY;
        if (expected_reward < MIN_REWARD_THRESHOLD)
        {
            penalty += SAFE_ZONE_BOUNDARY_PENALTY;
        }
    }

    return penalty;
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

// 修复的BFS输出结构
struct BFSOut
{
    int dist[H][W];
    int parent[H][W];
    int snake_cost[H][W];

    // 正确初始化
    BFSOut()
    {
        for (int i = 0; i < H; i++)
        {
            for (int j = 0; j < W; j++)
            {
                dist[i][j] = 1000000000; // 使用1e9而不是0x3f3f3f3f
                parent[i][j] = -1;
                snake_cost[i][j] = 0;
            }
        }
    }
};

// 修复的bfs_grid函数
BFSOut bfs_grid(const GridMask &M, const State &s, int sy, int sx, const Snake *snake_for_pathfinding = nullptr)
{
    BFSOut G;
    queue<Point> q;
    G.dist[sy][sx] = 0;
    q.push({sy, sx});

    // 获取当前方向，用于禁止掉头
    // 如果指定了特定蛇，使用其方向；否则使用我的蛇的方向
    const Snake &path_snake = snake_for_pathfinding ? *snake_for_pathfinding : s.self();
    int opposite_dir = (path_snake.dir + 2) % 4;

    while (!q.empty())
    {
        Point cur = q.front();
        q.pop();
        int cy = cur.y, cx = cur.x;
        int cd = G.dist[cy][cx];

        for (int k = 0; k < 4; ++k)
        {
            int ny = cy + DY[k], nx = cx + DX[k];
            if (!in_bounds(ny, nx))
                continue;

            // 简化：只考虑当前安全区内的移动
            if (!in_safe_zone(s.cur, ny, nx))
                continue;

            if (M.blocked(ny, nx))
                continue;

            // 如果是起始位置，禁止掉头方向
            if (cy == sy && cx == sx && k == opposite_dir)
                continue;

            // 计算新的距离
            int new_dist = cd + 1;

            // 陷阱额外代价
            if (M.is_trap(ny, nx))
            {
                new_dist += TRAP_STEP_COST;
            }

            // 蛇身穿越额外代价
            if (M.is_snake(ny, nx))
            {
                if (path_snake.shield_time > 0)
                {
                    // 有护盾，可以穿过
                }
                else if (can_open_shield(path_snake))
                {
                    new_dist += 1; // 开盾需要额外一回合
                }
                else
                {
                    continue; // 无法穿越，跳过
                }
            }

            // 更新距离和父节点
            if (new_dist < G.dist[ny][nx])
            {
                G.dist[ny][nx] = new_dist;
                G.parent[ny][nx] = k; // 记录从父节点到当前节点的方向
                q.push({ny, nx});
            }
        }
    }
    return G;
}

// 优势程度计算
double calculate_advantage(const Snake &me, const Snake &enemy, const Point &target, int my_dist, int enemy_dist, bool i_win_tie)
{
    double advantage = 0.0;
    advantage += (enemy_dist - my_dist) * 2.0;
    if (me.shield_time > enemy.shield_time)
        advantage += 3.0;
    else if (me.shield_time < enemy.shield_time)
        advantage -= 3.0;
    if (can_open_shield(me) && !can_open_shield(enemy))
        advantage += 2.0;
    else if (!can_open_shield(me) && can_open_shield(enemy))
        advantage -= 2.0;
    advantage += (me.length - enemy.length) * 0.1;
    if (my_dist == enemy_dist)
        advantage += i_win_tie ? 1.0 : -1.0;
    return advantage;
}

// 竞争检测函数
vector<CompetitionAnalysis> analyze_competition(const State &s, const GridMask &M, const BFSOut &G)
{
    vector<CompetitionAnalysis> competitions;
    const auto &me = s.self();
    for (const auto &item : s.items)
    {
        if (item.type < 1 && item.type != -1)
            continue;
        Point target = item.pos;
        int my_dist = G.dist[target.y][target.x];
        if (my_dist >= (int)1e9)
            continue;
        for (const auto &enemy : s.snakes)
        {
            if (enemy.id == MYID)
                continue;

            // Create a separate GridMask for this enemy that doesn't include their own body
            GridMask enemy_M;

            // Add all other snakes' bodies (but not this enemy's own body)
            for (const auto &other_sn : s.snakes)
            {
                if (other_sn.id == MYID || other_sn.id == enemy.id)
                    continue; // Skip self and the enemy we're calculating for

                for (const auto &p : other_sn.body)
                {
                    enemy_M.block(p.y, p.x);
                    enemy_M.snake(p.y, p.x);
                }

                // Mark danger zones around other snakes' heads
                auto other_head = other_sn.head();
                for (int k = 0; k < 4; k++)
                {
                    int ny = other_head.y + DY[k], nx = other_head.x + DX[k];
                    enemy_M.danger(ny, nx);
                }
            }

            // Add traps
            for (const auto &item : s.items)
            {
                if (item.type == -2) // 陷阱
                {
                    enemy_M.trap(item.pos.y, item.pos.x);
                }
            }

            // Add chests if enemy doesn't have key
            for (const auto &chest : s.chests)
            {
                if (!enemy.has_key)
                {
                    enemy_M.block(chest.pos.y, chest.pos.x);
                }
            }

            BFSOut enemy_bfs = bfs_grid(enemy_M, s, enemy.head().y, enemy.head().x, &enemy);
            int enemy_dist = enemy_bfs.dist[target.y][target.x];
            if (enemy_dist >= (int)1e9)
                continue;
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

// 完整的路径构建算法
void build_route_greedy(const GridMask &M, const State &s, const vector<Cand> &C, Point head, Route &R)
{
    R.seq.clear();
    R.totalVal = 0;
    R.finishT = 0;

    if (C.empty())
        return;

    Point cur_pos = head;
    int cur_time = 0;

    // 贪心选择最近的高价值目标
    for (const auto &cand : C)
    {
        BFSOut path = bfs_grid(M, s, cur_pos.y, cur_pos.x);
        int d = path.dist[cand.p.y][cand.p.x];

        // 检查可达性
        if (d >= (int)1e9)
            continue;

        // 检查时间限制
        if (cand.ttl != -1 && cur_time + d > cand.ttl)
            continue;

        // 检查安全区收缩
        bool valid = true;
        if (s.next_tick != -1 && cur_time + d >= s.next_tick)
        {
            if (!in_safe_zone(s.next, cand.p.y, cand.p.x))
            {
                valid = false;
            }
        }

        if (!valid)
            continue;

        // 添加到路径
        R.seq.push_back(cand.p);
        R.totalVal += cand.val;
        cur_time += d;
        cur_pos = cand.p;

        // 限制路径长度防止超时
        if (R.seq.size() >= 8)
            break;
    }

    R.finishT = cur_time;
}

void improve_route_ls(const GridMask &M, const State &s, Point head, Route &R)
{
    // 对于实时AI，路径改进应该简单快速且基于实际可达性
    // 由于第一个目标最重要，我们主要优化后续目标的顺序

    if (R.seq.size() <= 2) // 少于3个目标时无需优化
        return;

    // 改进策略：使用实际BFS距离而不是曼哈顿距离
    // 但限制计算量以保持实时性能

    if (R.seq.size() >= 3)
    {
        // 保持第一个目标不变，对剩余目标重新排序
        Point first_target = R.seq[0];

        // 从第一个目标位置计算到其他目标的实际距离
        BFSOut path_from_first = bfs_grid(M, s, first_target.y, first_target.x);

        // 构建剩余目标的候选列表，按实际距离排序
        vector<pair<int, int>> remaining; // 存储 (实际距离, 原索引)
        for (int i = 1; i < (int)R.seq.size(); i++)
        {
            int actual_dist = path_from_first.dist[R.seq[i].y][R.seq[i].x];
            // 如果无法到达，使用一个很大的距离值
            if (actual_dist >= (int)1e9)
            {
                actual_dist = 9999;
            }
            remaining.push_back({actual_dist, i});
        }

        // 按实际距离排序
        sort(remaining.begin(), remaining.end());

        // 重建路径序列
        vector<Point> new_seq;
        new_seq.push_back(first_target);
        for (const auto &item : remaining)
        {
            new_seq.push_back(R.seq[item.second]);
        }
        R.seq = new_seq;

        // 重新计算总时间（使用准确的BFS计算）
        int total_time = 0;
        Point cur_pos = head;
        for (const auto &target : R.seq)
        {
            BFSOut path = bfs_grid(M, s, cur_pos.y, cur_pos.x);
            int d = path.dist[target.y][target.x];
            if (d >= (int)1e9)
            {
                // 如果某个目标不可达，保持原路径
                return;
            }
            total_time += d;
            cur_pos = target;
        }
        R.finishT = total_time;
    }
}

// 完整的生存策略函数
int survival_strategy(const State &s, int sy, int sx, stringstream &log_ss)
{
    const auto &me = s.self();
    GridMask M;

    // 简化的掩码构建（重用逻辑）
    for (const auto &sn : s.snakes)
    {
        if (sn.id == MYID)
            continue;
        for (const auto &p : sn.body)
        {
            M.block(p.y, p.x);
        }
        auto head = sn.head();
        for (int k = 0; k < 4; k++)
        {
            int ny = head.y + DY[k], nx = head.x + DX[k];
            M.danger(ny, nx);
        }
    }

    log_ss << "SURVIVAL_STRATEGY:|";

    // 寻找最安全的移动方向
    int bestDir = -1;
    int bestReach = -1;

    for (int k = 0; k < 4; ++k)
    {
        int ny = sy + DY[k], nx = sx + DX[k];

        // 防止掉头
        int opposite_dir = (me.dir + 2) % 4;
        if (k == opposite_dir)
            continue;

        // 基本安全检查
        if (!in_bounds(ny, nx) || !in_safe_zone(s.cur, ny, nx) || M.blocked(ny, nx))
            continue;
        if (M.is_danger(ny, nx) && me.shield_time == 0)
            continue;

        // 计算可达性得分（简化版）
        int reachScore = 0;
        queue<Point> q;
        bool visited[H][W] = {false};
        q.push({ny, nx});
        visited[ny][nx] = true;

        while (!q.empty() && reachScore < 100) // 限制搜索范围
        {
            Point cur = q.front();
            q.pop();
            reachScore++;

            for (int dk = 0; dk < 4; dk++)
            {
                int nny = cur.y + DY[dk], nnx = cur.x + DX[dk];
                if (in_bounds(nny, nnx) && in_safe_zone(s.cur, nny, nnx) &&
                    !visited[nny][nnx] && !M.blocked(nny, nnx))
                {
                    visited[nny][nnx] = true;
                    q.push({nny, nnx});
                }
            }
        }

        if (reachScore > bestReach)
        {
            bestReach = reachScore;
            bestDir = k;
        }
    }

    if (bestDir != -1)
    {
        log_ss << "SURVIVAL_MOVE:DIR" << bestDir << "|";
        return ACT[bestDir];
    }

    // 尝试开盾
    if (me.shield_time == 0 && can_open_shield(me))
    {
        log_ss << "SURVIVAL_SHIELD|";
        return 4;
    }

    // 绝望移动：任何不掉头的方向
    int opposite_dir = (me.dir + 2) % 4;
    for (int k = 0; k < 4; ++k)
    {
        if (k == opposite_dir)
            continue;
        int ny = sy + DY[k], nx = sx + DX[k];
        if (in_bounds(ny, nx) && in_safe_zone(s.cur, ny, nx))
        {
            log_ss << "DESPERATE_DIR" << k << "|";
            return ACT[k];
        }
    }

    log_ss << "NO_SURVIVAL_OPTIONS|";
    return -1;
}

bool first_step_conflict(const State &s, int sy, int sx, int dir, bool opp_next[H][W])
{
    // 假设opp_next数组（需添加）
    int ny = sy + DY[dir], nx = sx + DX[dir];
    return opp_next[ny][nx];
}

// ==================== 决策函数 ====================
struct Choice
{
    int action;
};

Choice decide(const State &s)
{
    stringstream log_ss;
    const auto &me = s.self();
    int sy = me.head().y, sx = me.head().x;

    // 高优先：头已出区检查 - 智能返回安全区
    if (!in_safe_zone(s.cur, sy, sx) && me.shield_time == 0)
    {
        log_ss << "HEAD_OUTSIDE_SAFE:FINDING_RETURN_PATH|";

        // 先构建基本的GridMask来检查碰撞
        GridMask emergency_M;

        // 填充敌方蛇身（不包括自己的蛇）
        for (const auto &sn : s.snakes)
        {
            if (sn.id == MYID)
                continue; // 跳过自己的蛇，根据游戏规则自己不会碰撞
            for (const auto &p : sn.body)
            {
                emergency_M.block(p.y, p.x);
            }
        }

        // 寻找最快返回安全区的方向
        int best_dir = -1;
        int min_steps_to_safe = INT_MAX;
        int opposite_dir = (me.dir + 2) % 4;

        for (int k = 0; k < 4; k++)
        {
            // 严格禁止掉头（U-turn），这是蛇游戏的基本规则
            if (k == opposite_dir)
            {
                continue; // 绝对不允许掉头
            }

            int ny = sy + DY[k], nx = sx + DX[k];

            // 检查边界
            if (!in_bounds(ny, nx))
            {
                log_ss << "DIR" << k << ":OUT_OF_BOUNDS|";
                continue;
            }

            // 检查是否被敌方蛇身阻挡 (不使用GridMask.blocked因为它会阻挡安全区外的位置)
            bool blocked_by_enemy = false;
            if (in_bounds(ny, nx) && in_safe_zone(global_state.cur, ny, nx))
            {
                blocked_by_enemy = emergency_M.blocked(ny, nx);
            }
            else if (in_bounds(ny, nx))
            {
                // 对于安全区外的位置，手动检查是否有敌方蛇身
                for (const auto &sn : s.snakes)
                {
                    if (sn.id == MYID)
                        continue;
                    for (const auto &p : sn.body)
                    {
                        if (p.y == ny && p.x == nx)
                        {
                            blocked_by_enemy = true;
                            break;
                        }
                    }
                    if (blocked_by_enemy)
                        break;
                }
            }
            else
            {
                blocked_by_enemy = true; // 超出地图边界
            }

            if (blocked_by_enemy)
            {
                log_ss << "DIR" << k << ":BLOCKED_BY_ENEMY|";
                continue;
            }

            // 计算到安全区的距离
            int steps_to_safe = 0;
            if (in_safe_zone(s.cur, ny, nx))
            {
                steps_to_safe = 0; // 一步就能进入安全区
            }
            else
            {
                // 计算曼哈顿距离到安全区
                int dx = 0, dy = 0;
                if (nx < s.cur.x_min)
                    dx = s.cur.x_min - nx;
                else if (nx > s.cur.x_max)
                    dx = nx - s.cur.x_max;
                if (ny < s.cur.y_min)
                    dy = s.cur.y_min - ny;
                else if (ny > s.cur.y_max)
                    dy = ny - s.cur.y_max;
                steps_to_safe = dx + dy;
            }

            log_ss << "DIR" << k << ":(" << ny << "," << nx << ")steps=" << steps_to_safe << "|";

            if (steps_to_safe < min_steps_to_safe)
            {
                min_steps_to_safe = steps_to_safe;
                best_dir = k;
            }
        }
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

            log_ss << "EMERGENCY_RETURN:" << direction_name << ",steps:" << min_steps_to_safe << "|";
            str_info += log_ss.str();
            return {ACT[best_dir]};
        }
        else
        {
            // 如果所有方向都会掉头，选择任意非掉头方向
            for (int k = 0; k < 4; k++)
            {
                if (k != opposite_dir)
                {
                    log_ss << "EMERGENCY_ANY_DIR:" << k << "|";
                    str_info += log_ss.str();
                    return {ACT[k]};
                }
            }

            log_ss << "EMERGENCY_FORCED_LEFT|";
            str_info += log_ss.str();
            return {0};
        }
    }

    // 构建掩码（完整实现）
    GridMask M;

    // 填充敌方蛇身
    for (const auto &sn : s.snakes)
    {
        if (sn.id == MYID)
            continue; // 跳过自己

        // 标记敌方蛇身体为阻挡
        for (const auto &p : sn.body)
        {
            M.block(p.y, p.x);
            M.snake(p.y, p.x);
        }

        // 标记敌方蛇头周围为危险
        auto head = sn.head();
        for (int k = 0; k < 4; k++)
        {
            int ny = head.y + DY[k], nx = head.x + DX[k];
            M.danger(ny, nx);
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

    // 填充宝箱（如果没有钥匙则阻挡）
    for (const auto &chest : s.chests)
    {
        if (!me.has_key)
        {
            M.block(chest.pos.y, chest.pos.x);
        }
    }

    // 头撞头风险：填充敌方预测位置
    bool opp_next[H][W];
    memset(opp_next, false, sizeof(opp_next));

    for (const auto &sn : s.snakes)
    {
        if (sn.id == MYID)
            continue; // 跳过自己

        auto head = sn.head();
        // 预测敌蛇可能的下一步移动（当前方向 + 左转右转）
        for (int k = 0; k < 4; k++)
        {
            int ny = head.y + DY[k], nx = head.x + DX[k];
            if (in_bounds(ny, nx) && in_safe_zone(s.cur, ny, nx))
            {
                opp_next[ny][nx] = true;
            }
        }
    }

    // 如果头靠近边界，尝试返回安全区
    int dist_to_safe = danger_safe_zone(s.cur, sy, sx);
    if (dist_to_safe > 0)
    {
        log_ss << "NEAR_SHRINK_BOUNDARY:RETURN_TO_SAFE|";

        int min_distance = INT_MAX;
        int best_dir = -1;

        // 禁止掉头
        int opposite_dir_shrink = (me.dir + 2) % 4;

        for (int k = 0; k < 4; k++)
        {
            int ny = sy + DY[k], nx = sx + DX[k];

            // 防止掉头
            if (k == opposite_dir_shrink)
                continue;

            if (!in_bounds(ny, nx) || !in_safe_zone(s.cur, ny, nx) || M.blocked(ny, nx))
                continue;
            if (M.is_danger(ny, nx) && me.shield_time == 0)
                continue;

            int dist_to_safe = danger_safe_zone(s.cur, ny, nx);
            if (dist_to_safe < min_distance)
            {
                min_distance = dist_to_safe;
                best_dir = k;
            }
        }

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
            if (can_open_shield(me))
            {
                log_ss << "NO_SAFE_PATH:EMERGENCY_SHIELD|";
                str_info += log_ss.str();
                return {4};
            }
            else
            {
                log_ss << "NO_SAFE_PATH:TRYING_ANY_DIRECTION|";

                for (int k = 0; k < 4; k++)
                {
                    int ny = sy + DY[k], nx = sx + DX[k];

                    int opposite_dir = (me.dir + 2) % 4;
                    if (k == opposite_dir)
                        continue;

                    if (in_bounds(ny, nx) && in_safe_zone(s.cur, ny, nx))
                    {
                        if (first_step_conflict(s, sy, sx, k, opp_next) && me.shield_time == 0 && !can_open_shield(me))
                        {
                            int surv = survival_strategy(s, sy, sx, log_ss);
                            str_info += log_ss.str();
                            return {surv};
                        }
                        str_info += log_ss.str();
                        return {ACT[k]};
                    }
                }

                int surv = survival_strategy(s, sy, sx, log_ss);
                if (surv != -1)
                {
                    str_info += log_ss.str();
                    return {surv};
                }

                log_ss << "ULTIMATE_FALLBACK:FIND_NON_REVERSE|";

                int opposite_dir = (me.dir + 2) % 4;

                for (int fallback_k = 0; fallback_k < 4; fallback_k++)
                {
                    if (fallback_k != opposite_dir)
                    {
                        string fallback_direction;
                        switch (fallback_k)
                        {
                        case 0:
                            fallback_direction = "LEFT";
                            break;
                        case 1:
                            fallback_direction = "UP";
                            break;
                        case 2:
                            fallback_direction = "RIGHT";
                            break;
                        case 3:
                            fallback_direction = "DOWN";
                            break;
                        }

                        log_ss << "ULTIMATE_FALLBACK:" << fallback_direction << "|";
                        str_info += log_ss.str();
                        return {ACT[fallback_k]};
                    }
                }

                log_ss << "ULTIMATE_FALLBACK:FORCED_LEFT|";
                str_info += log_ss.str();
                return {0};
            }
        }
    }

    // 以当前蛇头为起点做一次全图 BFS
    BFSOut G = bfs_grid(M, s, sy, sx);
    Point head{sy, sx};

    // 候选集构建函数（扩展包括钥匙/宝箱/陷阱）
    auto build_candidates = [&](int K = 24)
    {
        vector<Cand> C;
        C.reserve(64);
        vector<CompetitionAnalysis> competitions = analyze_competition(s, M, G);
        map<pair<int, int>, CompetitionAnalysis> competition_map;
        for (const auto &comp : competitions)
        {
            pair<int, int> key = {comp.target.y, comp.target.x};
            // 保留最危险的竞争（最低优势或最近的敌人）
            if (competition_map.find(key) == competition_map.end() ||
                comp.enemy_dist < competition_map[key].enemy_dist ||
                (comp.enemy_dist == competition_map[key].enemy_dist && comp.advantage < competition_map[key].advantage))
            {
                competition_map[key] = comp;
            }
        }

        // Debug: Log competition analysis for food at (11,34)
        log_ss << "COMPETITION_DEBUG:|";
        for (const auto &comp : competitions)
        {
            if (comp.target.y == 11 && comp.target.x == 34)
            {
                log_ss << "FOOD(11,34):enemy" << comp.enemy_id
                       << ",my_d=" << comp.my_dist
                       << ",enemy_d=" << comp.enemy_dist
                       << ",i_win=" << (comp.i_win_tie ? "YES" : "NO")
                       << ",adv=" << (int)(comp.advantage * 100) << "|";
            }
        }
        for (const auto &it : s.items)
        {
            if (!in_bounds(it.pos.y, it.pos.x) || !in_safe_zone(s.cur, it.pos.y, it.pos.x))
                continue;
            int d = G.dist[it.pos.y][it.pos.x];
            if (d >= (int)1e9)
                continue;
            if (it.lifetime != -1 && d > it.lifetime)
                continue;

            // 检查是否应该避免这个食物目标
            bool should_avoid = false;
            int collision_risk_level = 0; // 碰撞风险等级：0=安全，1=低风险，2=中风险，3=高风险

            if (it.type >= 1 || it.type == -1) // 普通食物或成长食物
            {
                pair<int, int> target_key = {it.pos.y, it.pos.x};
                auto comp_it = competition_map.find(target_key);
                if (comp_it != competition_map.end())
                {
                    const CompetitionAnalysis &comp = comp_it->second;

                    // 更激进的碰撞风险评估
                    if (comp.enemy_dist <= 3) // 任何在3步内的敌人都视为高风险
                    {
                        collision_risk_level = 3;
                        should_avoid = true;
                    }
                    else if (comp.enemy_dist < comp.my_dist)
                    {
                        collision_risk_level = 3; // 敌人更近 = 高风险
                        should_avoid = true;
                    }
                    else if (comp.enemy_dist == comp.my_dist)
                    {
                        collision_risk_level = 3; // 任何距离相等的情况都视为高风险
                        should_avoid = true;
                    }
                    else if (comp.enemy_dist - comp.my_dist <= 2)
                    {
                        collision_risk_level = 2; // 优势少于2步 = 中风险
                        should_avoid = true;
                    }

                    // 根据优势度进一步调整风险等级
                    if (comp.advantage < -2.0)
                    {
                        collision_risk_level = max(collision_risk_level, 2);
                        should_avoid = true;
                    }
                    else if (comp.advantage < -1.0)
                    {
                        collision_risk_level = max(collision_risk_level, 1);
                    }

                    // 检查路径重叠风险：如果我和敌人的路径可能交叉
                    bool path_overlap_risk = false;
                    for (const auto &enemy : s.snakes)
                    {
                        if (enemy.id == MYID)
                            continue;

                        // 简化的路径重叠检测：检查是否在目标附近可能碰撞
                        int enemy_dist_to_target = comp.enemy_dist;
                        if (enemy_dist_to_target < (int)1e9 &&
                            abs(enemy_dist_to_target - d) <= 1 &&
                            manhattan(enemy.head().y, enemy.head().x, it.pos.y, it.pos.x) <= d + 2)
                        {
                            path_overlap_risk = true;
                            collision_risk_level = max(collision_risk_level, 2);
                            break;
                        }
                    }
                }
            }

            if (should_avoid)
            {
                // 记录避免的目标用于调试
                continue; // 跳过这个目标
            }

            int v = it.value;
            double base_score = (v * (it.lifetime == -1 ? 1.0 : pow(LIFETIME_SOFT_DECAY, d))) / (d + DISTANCE_OFFSET);

            // 根据碰撞风险等级调整基础评分
            double risk_penalty = 1.0;
            switch (collision_risk_level)
            {
            case 1:
                risk_penalty = 0.8;
                break; // 低风险：轻微降分
            case 2:
                risk_penalty = 0.6;
                break; // 中风险：明显降分
            case 3:
                risk_penalty = 0.3;
                break; // 高风险：大幅降分
            default:
                risk_penalty = 1.0;
                break; // 无风险：不降分
            }
            base_score *= risk_penalty;

            // ==================== 新增：路径选择偏好 ====================
            // 应用开阔区域奖励
            double original_score = base_score;
            base_score = apply_openness_bonus(M, it.pos.y, it.pos.x, base_score);

            // 计算并应用路径危险度惩罚
            int danger_penalty = calculate_path_danger_penalty(M, it.pos.y, it.pos.x, v);
            base_score -= danger_penalty;

            // 确保分数不会变成负数（除非原本就是负分的陷阱）
            if (v > 0 && base_score < 0)
            {
                base_score = 0.1; // 给予最小正分
            }

            // 记录路径偏好调整（仅对前几个候选目标）
            if (C.size() < 3)
            {
                double openness = calculate_openness(M, it.pos.y, it.pos.x);
                log_ss << "PATH_PREF:(" << it.pos.y << "," << it.pos.x << ")"
                       << "open=" << (int)(openness * 100)
                       << ",penalty=" << danger_penalty
                       << ",score:" << (int)(original_score * 100) << "->" << (int)(base_score * 100) << "|";
            }
            // ============================================================

            pair<int, int> target_key = {it.pos.y, it.pos.x};
            auto comp_it = competition_map.find(target_key);
            double final_score = base_score;
            if (comp_it != competition_map.end())
            {
                const CompetitionAnalysis &comp = comp_it->second;
                if (comp.advantage > 0)
                {
                    final_score *= (1.0 + comp.advantage * 0.2);
                    if (comp.advantage >= 3.0)
                        final_score *= 1.5;
                }
                else if (comp.advantage < -1.0)
                {
                    final_score *= (1.0 + comp.advantage * 0.3);
                    if (comp.advantage <= -3.0)
                        final_score *= 0.3;
                }
                else
                {
                    final_score *= (comp.i_win_tie ? 1.1 : 0.9);
                }
            }
            if (it.type == -3 && me.has_key)
                continue;
            if (it.type == -5 && !me.has_key)
                continue;
            C.push_back({it.pos, v, d, final_score, it.lifetime});
        }
        if (C.empty())
            return C;
        sort(C.begin(), C.end(), [](const Cand &a, const Cand &b)
             {
            if (a.score != b.score) return a.score > b.score;
            return a.d < b.d; });
        if ((int)C.size() > K)
            C.resize(K);
        return C;
    };

    auto C = build_candidates();
    log_ss << "CANDIDATES_COUNT:" << C.size() << "|";

    // 添加调试信息：显示前几个候选目标
    for (int i = 0; i < min(5, (int)C.size()); i++)
    {
        log_ss << "CAND" << i << ":(" << C[i].p.y << "," << C[i].p.x
               << ")d=" << C[i].d << ",v=" << C[i].val << ",s=" << (int)(C[i].score * 100) << "|";
    }

    auto last_choice = [&]()
    {
        log_ss << "SURVIVAL_MODE:|";

        // 寻找可达性最好的安全移动方向
        int bestDir = -1;
        int bestReach = -1;

        // 禁止掉头
        int opposite_dir = (me.dir + 2) % 4;

        for (int k = 0; k < 4; ++k)
        {
            int ny = sy + DY[k], nx = sx + DX[k];

            // 防止掉头
            if (k == opposite_dir)
                continue;

            if (!in_bounds(ny, nx) || !in_safe_zone(s.cur, ny, nx) || M.blocked(ny, nx))
                continue;
            if (M.is_danger(ny, nx) && me.shield_time == 0)
                continue;

            // 计算可达性得分
            BFSOut tempG = bfs_grid(M, s, ny, nx);
            int reachScore = 0;
            for (int y = 0; y < H; ++y)
                for (int x = 0; x < W; ++x)
                    if (tempG.dist[y][x] < (int)1e9)
                        reachScore++;

            // ==================== 新增：路径偏好评估 ====================
            // 应用开阔度奖励（生存模式中这更重要）
            double openness_score = calculate_openness(M, ny, nx);
            reachScore = (int)(reachScore * (1.0 + openness_score * 0.5));

            // 检查并惩罚危险路径
            if (is_dead_end(M, ny, nx))
                reachScore -= 500; // 严重惩罚死路
            if (is_narrow_path(M, ny, nx))
                reachScore -= 200; // 惩罚狭窄路径
            if (near_safe_zone_boundary(ny, nx))
                reachScore -= 300; // 惩罚安全区边界
            // ============================================================

            if (reachScore > bestReach)
            {
                bestReach = reachScore;
                bestDir = k;
            }
        }

        if (bestDir != -1)
        {
            log_ss << "SURVIVAL_MOVE:DIR" << bestDir << "|";
            str_info += log_ss.str();
            return ACT[bestDir];
        }

        // 尝试开盾
        if (me.shield_time == 0 && can_open_shield(me))
        {
            log_ss << "SURVIVAL_SHIELD|";
            str_info += log_ss.str();
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
            str_info += log_ss.str();
            return ACT[k];
        }

        log_ss << "FORCED_FALLBACK|";
        str_info += log_ss.str();
        return 0;
    };

    // 如果没有候选目标，使用后备策略
    if (C.empty())
    {
        int choice = last_choice();
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
        int choice = last_choice();
        return {choice};
    }

    // 发出朝向第一个目标的第一步
    auto goal = R.seq.front();

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

            if (enemy_dist < (int)1e9)
            {
                // 如果敌人距离更近，或距离相等但敌人在平局中获胜
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

            // 遍历所有候选目标，寻找最安全的替代方案
            for (const auto &candidate : C)
            {
                // 跳过当前争夺的目标
                if (candidate.p.y == goal.y && candidate.p.x == goal.x)
                    continue;

                int my_dist_to_alt = G.dist[candidate.p.y][candidate.p.x];
                if (my_dist_to_alt >= (int)1e9)
                    continue;

                // 计算安全评分：考虑敌人到此目标的最小距离
                int min_enemy_dist = INT_MAX;
                bool any_enemy_closer = false;

                for (const auto &enemy : s.snakes)
                {
                    if (enemy.id == MYID)
                        continue;

                    BFSOut enemy_G_alt = bfs_grid(M, s, enemy.head().y, enemy.head().x);
                    int enemy_dist_to_alt = enemy_G_alt.dist[candidate.p.y][candidate.p.x];

                    if (enemy_dist_to_alt < (int)1e9)
                    {
                        min_enemy_dist = min(min_enemy_dist, enemy_dist_to_alt);
                        if (enemy_dist_to_alt <= my_dist_to_alt)
                        {
                            any_enemy_closer = true;
                        }
                    }
                }

                // 如果有敌人也能更快到达此目标，跳过
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
                // 如果所有备选目标都不安全，尝试序列中的下一个
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

                        // 检查3x3区域内的敌人威胁
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

                        // 敌人威胁越少，评分越高
                        area_score = 100.0 - enemy_threats * 10.0;

                        // 计算该方向的可达性奖励
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

                        // ==================== 新增：路径偏好评估 ====================
                        // 应用开阔度奖励
                        double openness = calculate_openness(M, ny, nx);
                        area_score *= (1.0 + openness * 0.3); // 30%的开阔度奖励

                        // 应用危险路径惩罚
                        int danger_penalty = calculate_path_danger_penalty(M, ny, nx, 50); // 假设中等奖励
                        area_score -= danger_penalty * 0.5;                                // 减轻惩罚以避免过度影响
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

                    log_ss << "NO_SAFE_AREA:USING_SURVIVAL_MODE|";
                    int choice = last_choice();
                    return {choice};
                }
            }
        }
    }

    auto G2 = bfs_grid(M, s, head.y, head.x);
    if (G2.parent[goal.y][goal.x] == -1)
    {
        int choice = last_choice();
        return {choice};
    }

    // 重构路径：从目标回溯到起点，找到第一步方向
    if (G2.parent[goal.y][goal.x] == -1)
    {
        int choice = last_choice();
        return {choice};
    }

    // 路径重构：从目标回溯到起点
    int dir = -1;

    // 如果距离为1，直接计算方向
    if (G2.dist[goal.y][goal.x] == 1)
    {
        for (int k = 0; k < 4; k++)
        {
            if (head.y + DY[k] == goal.y && head.x + DX[k] == goal.x)
            {
                dir = k;
                break;
            }
        }
    }
    else
    {
        // 距离大于1，回溯找到第一步
        int cy = goal.y, cx = goal.x;

        // 回溯到距离起点为1的位置
        while (G2.dist[cy][cx] > 1)
        {
            int parent_dir = G2.parent[cy][cx];
            // parent_dir是从父节点到当前节点的方向，需要反向找父节点
            int parent_y = cy - DY[parent_dir];
            int parent_x = cx - DX[parent_dir];
            cy = parent_y;
            cx = parent_x;
        }

        // 现在(cy,cx)是距离起点1步的位置，计算从起点到此位置的方向
        for (int k = 0; k < 4; k++)
        {
            if (head.y + DY[k] == cy && head.x + DX[k] == cx)
            {
                dir = k;
                break;
            }
        }
    }

    if (dir == -1)
    {
        int choice = last_choice();
        return {choice};
    }

    // 检查原始路径的第一步是否安全，如果安全就使用原始路径
    int ny_original = head.y + DY[dir], nx_original = head.x + DX[dir];
    bool original_safe = in_bounds(ny_original, nx_original) &&
                         !M.blocked(ny_original, nx_original) &&
                         !M.is_danger(ny_original, nx_original);

    // ==================== 新增：路径偏好安全检查 ====================
    // 即使路径"安全"，也要检查是否过于危险（死路等）
    bool path_preference_safe = true;
    if (original_safe)
    {
        int danger_penalty = calculate_path_danger_penalty(M, ny_original, nx_original, 50);
        // 如果危险度惩罚过高，认为路径不够安全
        if (danger_penalty > 250) // 高危险阈值
        {
            path_preference_safe = false;
            log_ss << "PATH_TOO_DANGEROUS:penalty=" << danger_penalty << "|";
        }
    }
    // ============================================================

    // 如果原始路径不安全或危险度过高，才寻找替代方案
    if (!original_safe || !path_preference_safe)
    {
        int target_dist = G2.dist[goal.y][goal.x];
        int opposite_dir_alt = (me.dir + 2) % 4;
        int best_alternative_dir = -1;
        double best_alternative_score = -1000.0;

        for (int k = 0; k < 4; k++)
        {
            if (k == dir)
                continue; // 跳过已经检查过的原始方向

            // 防止掉头
            if (k == opposite_dir_alt)
                continue;

            int ny = head.y + DY[k], nx = head.x + DX[k];
            if (!in_bounds(ny, nx) || M.blocked(ny, nx))
                continue;

            // 只有当这个位置能够到达目标时才考虑
            if (G2.dist[ny][nx] < 1000000000 &&
                G2.dist[ny][nx] + manhattan(ny, nx, goal.y, goal.x) <= target_dist + 1)
            {
                if (!M.is_danger(ny, nx))
                {
                    // ==================== 新增：路径偏好评估替代方案 ====================
                    double alternative_score = 100.0; // 基础分数

                    // 应用开阔度奖励
                    double openness = calculate_openness(M, ny, nx);
                    alternative_score *= (1.0 + openness * 0.4);

                    // 扣除危险路径惩罚
                    int danger_penalty = calculate_path_danger_penalty(M, ny, nx, 50);
                    alternative_score -= danger_penalty;

                    // 选择评分最高的替代方案
                    if (alternative_score > best_alternative_score)
                    {
                        best_alternative_score = alternative_score;
                        best_alternative_dir = k;
                    }
                    // ============================================================
                }
            }
        }

        // 如果找到了更好的替代方案，使用它
        if (best_alternative_dir != -1)
        {
            dir = best_alternative_dir;
            log_ss << "USING_SAFER_ALTERNATIVE:dir=" << dir
                   << ",score=" << (int)best_alternative_score << "|";
        }
    }

    // 防止180度掉头检查
    int opposite_dir = (me.dir + 2) % 4;
    if (dir == opposite_dir)
    {
        log_ss << "ERROR:REVERSE_DIRECTION_BLOCKED|";
        str_info += log_ss.str();
        int choice = last_choice();
        return {choice};
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

    log_ss << "MULTI_TARGET_MOVE:" << next_direction << ",a:" << ACT[dir] << "|";
    str_info += log_ss.str();
    return {ACT[dir]};
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
    // freopen("D:\\su25-program\\snake\\input.in", "r", stdin); // 调试用，实际使用时注释掉
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    read_state(global_state);
    auto choice = decide(global_state);
    cout << choice.action << "\n";
    // cout << str_info << "\n";
    return 0;
}