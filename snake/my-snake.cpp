#include <bits/stdc++.h>
using namespace std;

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
static constexpr int GROWTH_FOOD_VALUE = 10;     // 成长食物价值
static constexpr int TRAP_PENALTY = -10;         // 陷阱扣分（负值）
static constexpr int KEY_VALUE = 75;             // 钥匙价值
static constexpr int CHEST_VALUE = 100;          // 宝箱基础价值
static constexpr int NORMAL_FOOD_MULTIPLIER = 4; // 普通食物价值倍数

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
 * - 正数: 普w通食物的分值
 * - -1: 成长食物（使蛇身变长）
 * - -2: 陷阱（有害） //扣20分
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
        switch (s.items[i].type)
        {
        case -1: // 成长食物
            s.items[i].value = GROWTH_FOOD_VALUE;
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
            s.items[i].value = s.items[i].type * NORMAL_FOOD_MULTIPLIER;
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

    // 可选的内存行被忽略
    // cin >> str_info;
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

// 游戏引擎动作映射 (按规范): 0=左,1=上,2=右,3=下,4=护盾
static const int DX[4] = {-1, 0, 1, 0}; // x方向偏移：左、上、右、下
static const int DY[4] = {0, -1, 0, 1}; // y方向偏移：左、上、右、下
static const int ACT[4] = {0, 1, 2, 3}; // 索引0..3 -> 动作代码

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

// ==================== 地图掩码构建 ====================

/**
 * 为当前回合构建阻挡和危险位置掩码
 *
 * 阻挡位置包括：
 * 1. 安全区域外的位置
 * 2. 其他蛇的身体
 * 3. 宝箱位置 (如果自己没有钥匙)
 *
 * 危险位置包括：
 * 4. 敌蛇头部相邻的位置（预测敌蛇下一步可能的位置）
 */
static GridMask build_masks(const State &s)
{
    GridMask M;

    // 2) 所有蛇的身体 = 阻挡
    for (const auto &sn : s.snakes)
    {
        // 自己的蛇
        if (sn.id == MYID)
        {
            continue;
        }
        else
        { // 其他蛇全部身体阻挡
            for (const auto &p : sn.body)
                if (in_bounds(p.y, p.x) && in_safe_zone(s.cur, p.y, p.x))
                {
                    M.snake(p.y, p.x); // 保留诊断信息
                    M.block(p.y, p.x); // 新增：设为硬障碍
                }
        }
    }

    // 3) 宝箱 = 阻挡障碍物（总是阻挡，忽略作为目标）
    for (const auto &c : s.chests)
    {
        if (in_bounds(c.pos.y, c.pos.x) && in_safe_zone(s.cur, c.pos.y, c.pos.x))
            M.block(c.pos.y, c.pos.x); // 总是阻挡
    }

    // 4) 从物品中提取陷阱位置并标记（可以通过但不推荐）
    for (const auto &item : s.items)
    {
        if (item.type == -2) // 陷阱
        {
            if (in_bounds(item.pos.y, item.pos.x) && in_safe_zone(s.cur, item.pos.y, item.pos.x))
                M.trap(item.pos.y, item.pos.x);
        }
    }

    // 5) 预测敌蛇头部邻居 = 危险位置
    for (const auto &sn : s.snakes)
    {
        if (sn.id == MYID) // 跳过自己的蛇
            continue;
        auto h = sn.head();
        // 标记敌蛇头部四周的位置为危险
        for (int k = 0; k < 4; k++)
        {
            int ny = h.y + DY[k], nx = h.x + DX[k];
            // 考虑敌蛇下一步可能移动到的位置
            // 如果我方蛇没有护盾，需要避开这些潜在的敌蛇头部位置

            if (in_bounds(ny, nx) && in_safe_zone(s.cur, ny, nx))
                M.danger(ny, nx);
        }
    }
    return M;
}

inline bool can_open_shield()
{
    if (global_state.self().shield_cd == 0 && global_state.self().score >= SHIELD_COST_THRESHOLD)
        return true;
    return false;
}

// ==================== 广度优先搜索 (BFS) ====================

/**
 * BFS输出结构
 * 存储距离和路径信息
 */
struct BFSOut
{
    array<array<int, W>, H> dist;       // 距离矩阵 dist[y][x]
    array<array<int, W>, H> snake_cost; // 穿过蛇身的代价 snake_cost[y][x]
    array<array<int, W>, H> parent;     // 父节点方向 parent[y][x]
};

/**
 * 在网格上执行BFS，从起始位置(sy, sx)开始
 * 计算到所有可达位置的最短距离和路径
 *
 * @param M 网格掩码，包含阻挡信息
 * @param sy 起始y坐标
 * @param sx 起始x坐标
 * @return BFS结果，包含距离和父节点信息
 */
static BFSOut bfs_grid(const GridMask &M, const State &s, int sy, int sx)
{
    BFSOut out;

    // 初始化距离为无穷大，父节点为-1
    for (int y = 0; y < H; y++)
        for (int x = 0; x < W; x++)
        {
            out.dist[y][x] = 1e9;
            out.snake_cost[y][x] = 0;
            out.parent[y][x] = -1;
        }

    // 使用优先队列进行带权重的搜索
    priority_queue<tuple<int, int, int, int>, vector<tuple<int, int, int, int>>, greater<>> pq;

    // 起始位置
    out.dist[sy][sx] = 0;
    out.snake_cost[sy][sx] = 0;
    pq.emplace(0, 0, sy, sx); // (总代价, 蛇身代价, y, x)

    // BFS主循环
    while (!pq.empty())
    {
        auto [total_cost, snake_steps, y, x] = pq.top(); // 总代价, 蛇身代价, y, x
        pq.pop();

        // 如果已经找到更好的路径，跳过
        if (total_cost > out.dist[y][x] + out.snake_cost[y][x] * SNAKE_COST_WEIGHT)
            continue;

        // 尝试四个方向
        for (int k = 0; k < 4; k++)
        {
            int ny = y + DY[k], nx = x + DX[k];

            // 检查边界和阻挡 必死检查
            if (!(in_bounds(ny, nx)) || M.blocked(ny, nx))
                continue;

            int new_dist = out.dist[y][x] + 1;
            int new_snake_cost = out.snake_cost[y][x];

            // 如果是蛇身格子/安全区外，增加蛇身代价 （有代价的步骤）
            if (M.is_snake(ny, nx) || !in_safe_zone(s.cur, ny, nx))
            {
                // 只有"已经在护盾状态中"才把穿过蛇身当作低成本；
                // 不能因为"可以开启护盾"而立刻低成本，因为开启护盾会消耗一个回合（原地不动）。
                if (s.snakes[s.self_idx].shield_time > 0)
                {
                    new_snake_cost += SNAKE_COST_WITH_SHIELD; // 有护盾时的低代价
                }
                else if (can_open_shield()) // 没有护盾但可开护盾
                {
                    new_snake_cost += SNAKE_COST_NO_SHIELD; // 当前无护盾：穿过蛇身代价极高（尽量避免）
                    new_dist += 1;                          // 开启护盾需要消耗一个回合
                }
                else
                {                            // 没有也不可开
                    new_snake_cost += 10000; // 当前无护盾：穿过蛇身代价极急急急高（尽量避免）
                }
            }

            // 计算陷阱惩罚 - 软性避开陷阱但不完全禁止通过
            int extra_cost = 0;
            if (M.is_trap(ny, nx))
            {
                extra_cost += TRAP_STEP_COST * 2; // 对陷阱格子施加额外代价，使路径规划倾向于避开
            }

            int new_total_cost = new_dist + new_snake_cost * SNAKE_COST_WEIGHT + extra_cost;

            // 如果找到更好的路径，更新
            if (new_total_cost < out.dist[ny][nx] + out.snake_cost[ny][nx] * SNAKE_COST_WEIGHT)
            {
                out.dist[ny][nx] = new_dist;
                out.snake_cost[ny][nx] = new_snake_cost;
                out.parent[ny][nx] = (k + 2) % 4;
                pq.emplace(new_total_cost, new_snake_cost, ny, nx);
            }
        }
    }
    return out;
}

// ==================== 路径规划辅助函数 ====================

/**
 * 曼哈顿距离计算
 */
inline int manhattan(Point a, Point b)
{
    return abs(a.y - b.y) + abs(a.x - b.x);
}

/**
 * 从任意起点计算到目标的网格距离
 */
inline int dist_grid_steps(const GridMask &M, const State &s, Point from, Point to)
{
    if (from.y == to.y && from.x == to.x)
        return 0;
    auto G = bfs_grid(M, s, from.y, from.x);
    int d = G.dist[to.y][to.x];
    return (d >= (int)1e9) ? (int)1e9 : d;
}

/**
 * 模拟路径后缀（从索引L到结尾）
 */
static bool simulate_suffix(const GridMask &M, const State &s,
                            const vector<Point> &seq,
                            int L,                 // 第一个需要重新计算的索引
                            Point head,            // 当前头部位置
                            const Timeline &TL_in, // 之前的时间线（用于前缀）
                            Timeline &TL_out,
                            int & /*totalVal*/, int &finishT)
{
    TL_out = TL_in;
    if ((int)TL_out.arrive.size() != (int)seq.size())
    {
        TL_out.arrive.resize(seq.size());
        TL_out.leave.resize(seq.size());
    }
    Point cur = head;
    int t = 0;
    if (L > 0)
    {
        cur = seq[L - 1];
        t = TL_in.leave[L - 1];
    }
    for (int i = L; i < (int)seq.size(); ++i)
    {
        int d = dist_grid_steps(M, s, cur, seq[i]);
        if (d >= (int)1e9)
        {
            TL_out.feasible = false;
            return false;
        }
        int arr = t + d;
        TL_out.arrive[i] = arr;
        TL_out.leave[i] = arr; // 拾取代价 ~ 0
        t = arr;
        cur = seq[i];
    }
    TL_out.feasible = true;
    finishT = (seq.empty() ? 0 : TL_out.leave.back());
    return true;
}

/**
 * 贪婪可行插入构建路径
 */
static void build_route_greedy(const GridMask &M, const State &s,
                               const vector<Cand> &C,
                               Point head, Route &R)
{
    R.seq.clear();
    R.totalVal = 0;
    R.finishT = 0;
    Timeline TL;
    TL.arrive.clear();
    TL.leave.clear();
    TL.feasible = true;

    vector<Point> seq;
    for (const auto &c : C)
    {
        int bestPos = -1;
        double bestGain = -1;
        Timeline TL_try;
        int bestFinish = 0;

        for (int pos = 0; pos <= (int)seq.size(); ++pos)
        {
            vector<Point> trial = seq;
            trial.insert(trial.begin() + pos, c.p);
            Timeline TL_out = TL;
            int totalVal = 0, finT = 0;
            if (!simulate_suffix(M, s, trial, pos, head, TL, TL_out, totalVal, finT))
                continue;

            int oldFin = (seq.empty() ? 0 : (TL.leave.empty() ? 0 : TL.leave.back()));
            int dVal = c.val;
            int dTime = finT - oldFin;
            double gain = (dTime > 0 ? (double)dVal / dTime : (dVal > 0 ? 1e9 : 0));
            if (gain > bestGain)
            {
                bestGain = gain;
                bestPos = pos;
                bestFinish = finT;
                TL_try = TL_out;
            }
        }
        if (bestPos != -1)
        {
            seq.insert(seq.begin() + bestPos, c.p);
            TL = TL_try;
            R.totalVal += c.val;
            R.finishT = bestFinish;
        }
    }
    R.seq = std::move(seq);
}

/**
 * 短距离局部搜索改进路径
 */
static void improve_route_ls(const GridMask &M, const State &s, Point head, Route &R)
{
    if (R.seq.size() < 2)
        return;
    Timeline TL;
    TL.arrive.resize(R.seq.size());
    TL.leave.resize(R.seq.size());
    TL.feasible = true;

    int dummy = 0, finT = 0;
    if (!simulate_suffix(M, s, R.seq, 0, head, TL, TL, dummy, finT))
        return;
    R.finishT = finT;

    auto better = [&](int newVal, int newFin)
    {
        if (newVal != R.totalVal)
            return newVal > R.totalVal;
        return newFin < R.finishT;
    };

    const auto tStart = chrono::high_resolution_clock::now();
    int tries = 0;

    while (true)
    {
        bool improved = false;

        // 重定位操作
        for (int i = 0; i < (int)R.seq.size() && !improved; ++i)
        {
            for (int j = 0; j < (int)R.seq.size() && !improved; ++j)
            {
                if (i == j)
                    continue;
                auto seq = R.seq;
                Point moved = seq[i];
                seq.erase(seq.begin() + i);
                seq.insert(seq.begin() + (j < i ? j : j), moved);
                Timeline TL2;
                int dummy2 = 0, finT2 = 0;
                if (!simulate_suffix(M, s, seq, min(i, j), head, TL, TL2, dummy2, finT2))
                    continue;
                int newVal = R.totalVal; // 重新排序不改变价值总和
                if (better(newVal, finT2))
                {
                    R.seq = std::move(seq);
                    TL = TL2;
                    R.finishT = finT2;
                    improved = true;
                    break;
                }
                if (++tries > 400)
                    break;
            }
        }
        if (improved)
            continue;

        // 交换操作
        for (int i = 0; i + 1 < (int)R.seq.size() && !improved; ++i)
        {
            for (int j = i + 1; j < (int)R.seq.size() && !improved; ++j)
            {
                auto seq = R.seq;
                swap(seq[i], seq[j]);
                Timeline TL2;
                int dummy2 = 0, finT2 = 0;
                if (!simulate_suffix(M, s, seq, min(i, j), head, TL, TL2, dummy2, finT2))
                    continue;
                int newVal = R.totalVal;
                if (better(newVal, finT2))
                {
                    R.seq = std::move(seq);
                    TL = TL2;
                    R.finishT = finT2;
                    improved = true;
                    break;
                }
                if (++tries > 400)
                    break;
            }
        }
        if (improved)
            continue;

        // 时间/尝试次数限制
        auto ms = chrono::duration_cast<chrono::milliseconds>(
                      chrono::high_resolution_clock::now() - tStart)
                      .count();
        if (ms > 550 || tries > 400)
            break; // 保持整个tick ≤ 0.7s
        break;     // 没有找到改进
    }
}

// ==================== 决策算法 ====================

/**
 * 选择结构
 */
struct Choice
{
    int action; // 选择的动作
};

/**
 * 主决策函数
 * 基于当前游戏状态做出最优移动决策
 *
 * 算法流程：
 * 1. 构建地图掩码（阻挡和危险位置）
 * 2. 从蛇头位置执行BFS
 * 3. 评估所有可达的食物目标
 * 4. 选择最优目标并计算第一步移动
 * 5. 如果没有好目标，使用后备策略
 */
static Choice decide(const State &s)
{
    const auto &me = s.self();
    GridMask M = build_masks(s);                  // 1) 构建阻挡 / 危险掩码
    const int sy = me.head().y, sx = me.head().x; // 当前蛇头坐标

    stringstream log_ss; // 使用 stringstream 高效构建日志
    log_ss << "TICK:" << s.current_ticks << "|"
           << "POSITION:" << sy << "," << sx << "|"
           << "SCORE:" << me.score << "|"
           << "LENGTH:" << me.length << "|"
           << "SHIELD_COOLDOWN:" << me.shield_cd << "|"
           << "SHIELD_TIME:" << me.shield_time << "|";

    // 敌人到任意点的粗略距离（曼哈顿距离近似）
    // auto min_opp_dist = [&](int y, int x) -> int
    // {
    //     int best = 1000000000;
    //     for (const auto &sn : s.snakes)
    //     {
    //         if (sn.id == MYID)
    //             continue;
    //         const auto &h = sn.head();
    //         if (h.y < 0 || h.y >= H || h.x < 0 || h.x >= W)
    //             continue;
    //         int md = std::abs(y - h.y) + std::abs(x - h.x);
    //         if (md < best)
    //             best = md;
    //     }
    //     return best;
    // };

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

            // 检查是否有障碍物（包括自己的身体和敌方蛇体）
            bool blocked = false;

            // 检查自己的身体碰撞（除了尾巴可能会移动）
            for (int i = 0; i < me.body.size() - 1; i++)
            {
                if (me.body[i].y == ny && me.body[i].x == nx)
                {
                    blocked = true;
                    break;
                }
            }

            // 检查敌方蛇体碰撞
            if (!blocked)
            {
                for (const auto &enemy : s.snakes)
                {
                    if (enemy.id == MYID)
                        continue;
                    for (const auto &p : enemy.body)
                    {
                        if (p.y == ny && p.x == nx)
                        {
                            blocked = true;
                            break;
                        }
                    }
                    if (blocked)
                        break;
                }
            }

            // 检查掉头
            int opposite_dir = (me.dir + 2) % 4;
            if (k == opposite_dir)
            {
                blocked = true;
            }

            if (blocked)
            {
                log_ss << "DIR" << k << ":BLOCKED|";
                continue;
            }

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

                // 如果连基本移动都不可能，尝试找一个不会掉头的方向
                log_ss << "ULTIMATE_FALLBACK:FIND_NON_REVERSE|";

                // 计算掉头方向
                int opposite_dir = (me.dir + 2) % 4;

                // 尝试四个方向，找到第一个不会掉头的方向
                for (int fallback_k = 0; fallback_k < 4; fallback_k++)
                {
                    if (fallback_k != opposite_dir) // 不是掉头方向
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

                // 如果所有方向都是掉头（理论上不可能），只能硬选一个
                log_ss << "ULTIMATE_FALLBACK:FORCED_LEFT|";
                str_info += log_ss.str();
                return {0};
            }
        }
    }

    // 2) 预计算敌蛇头部可能的下一步位置的权值（用于头撞头风险检测）
    int opp_next[H][W];
    memset(opp_next, 0, sizeof(opp_next));
    for (const auto &sn : s.snakes)
    {
        if (sn.id == MYID)
            continue; // 跳过自己
        auto head = sn.head();
        // 预测敌蛇头部四个方向的可能位置
        for (int k = 0; k < 4; k++)
        {
            int ny = head.y + DY[k], nx = head.x + DX[k];
            if (in_bounds(ny, nx) && in_safe_zone(s.cur, ny, nx))
            {
                opp_next[ny][nx] = true;
            }
        }
    }

    // 3) 以当前蛇头为起点做一次全图 BFS
    BFSOut G = bfs_grid(M, s, sy, sx);
    Point head{sy, sx};

    // 候选集构建函数（仅食物和成长食物）
    auto build_candidates = [&](int K = 24)
    {
        vector<Cand> C;
        C.reserve(64);
        for (const auto &it : s.items)
        {
            // 忽略非食物：钥匙(-3)、宝箱(-5)、陷阱(-2)
            if (!(it.type >= 1 || it.type == -1))
                continue;

            // 可达性和生命周期检查
            if (!in_bounds(it.pos.y, it.pos.x) || !in_safe_zone(s.cur, it.pos.y, it.pos.x))
                continue;
            int d = G.dist[it.pos.y][it.pos.x];
            if (d >= (int)1e9)
                continue;
            if (it.lifetime != -1 && d > it.lifetime)
                continue;

            int v = (it.type >= 1) ? (it.type * NORMAL_FOOD_MULTIPLIER) : GROWTH_FOOD_VALUE;
            double life_factor = (it.lifetime == -1 ? 1.0 : pow(LIFETIME_SOFT_DECAY, d));
            double sc = (v * life_factor) / (d + DISTANCE_OFFSET);
            C.push_back({it.pos, v, d, sc, it.lifetime});
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

    auto last_choice = [&]()
    {
        log_ss << "SURVIVAL_MODE:|";

        // 寻找可达性最好的安全移动方向
        int bestDir = -1;
        int bestReach = -1;

        for (int k = 0; k < 4; ++k)
        {
            int ny = sy + DY[k], nx = sx + DX[k];

            if (!in_bounds(ny, nx) || !in_safe_zone(s.cur, ny, nx) || M.blocked(ny, nx))
                continue;
            if (M.is_danger(ny, nx) && me.shield_time == 0)
                continue;

            // 计算可达性得分
            int reachScore = 0;
            BFSOut tempG = bfs_grid(M, s, ny, nx);
            for (int y = 0; y < H; ++y)
                for (int x = 0; x < W; ++x)
                    if (tempG.dist[y][x] < (int)1e9)
                        reachScore++;

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
        if (me.shield_time == 0 && can_open_shield())
        {
            log_ss << "SURVIVAL_SHIELD|";
            str_info += log_ss.str();
            return 4;
        }

        // 绝望移动
        for (int k = 0; k < 4; ++k)
        {
            int ny = sy + DY[k], nx = sx + DX[k];
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
    auto G2 = bfs_grid(M, s, head.y, head.x);
    if (G2.parent[goal.y][goal.x] == -1)
    {
        int choice = last_choice();
        return {choice};
    }

    // 重构一步（优先选择非危险邻居）
    int cy = goal.y, cx = goal.x;
    while (!(cy == head.y && cx == head.x))
    {
        int back = G2.parent[cy][cx];
        int py = cy + DY[back], px = cx + DX[back];
        if (py == head.y && px == head.x)
            break;
        cy = py;
        cx = px;
    }

    int dir = -1;
    for (int k = 0; k < 4; k++)
        if (head.y + DY[k] == cy && head.x + DX[k] == cx)
        {
            dir = k;
            break;
        }

    if (dir == -1)
    {
        int choice = last_choice();
        return {choice};
    }

    // 可选：在相等最短路径中选择非危险邻居
    for (int k = 0; k < 4; k++)
    {
        int ny = head.y + DY[k], nx = head.x + DX[k];
        if (!in_bounds(ny, nx) || M.blocked(ny, nx))
            continue;
        if (G2.dist[ny][nx] == G2.dist[goal.y][goal.x] - 1 && !M.is_danger(ny, nx))
        {
            dir = k;
            break;
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
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    read_state(global_state);
    auto choice = decide(global_state);
    cout << choice.action << "\n";
    cout << str_info << "\n";
    return 0;
}
