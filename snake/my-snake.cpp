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
    int current_ticks;
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
            s.items[i].value = 5;
            break;
        case -2: // 陷阱 - 根据游戏规则扣除10分
            s.items[i].value = -10;
            break;
        case -3: // 钥匙
            s.items[i].value = 10;
            break;
        case -5: // 宝箱
            s.items[i].value = 100;
            break;
        default: // 普通食物
            s.items[i].value = s.items[i].type * 3;
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
 * 检查坐标是否在安全区域内（同时检查地图边界）
 */
inline bool in_safe(const Safe &z, int y, int x)
{
    // 首先检查地图边界
    if (!(0 <= y && y < H && 0 <= x && x < W))
        return false;
    // 然后检查安全区域
    return x >= z.x_min && x <= z.x_max && y >= z.y_min && y <= z.y_max;
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
        if (in_safe(global_state.cur, y, x))
            blocked_rows[y].set(x);
    }

    /**
     * 标记位置为敌方蛇身体
     */
    inline void snake(int y, int x)
    {
        if (in_safe(global_state.cur, y, x))
            snake_rows[y].set(x);
    }

    /**
     * 标记位置为危险-蛇头周围
     */
    inline void danger(int y, int x)
    {
        if (in_safe(global_state.cur, y, x))
            danger_rows[y].set(x);
    }
    /**
     * 标记位置为陷阱
     */
    inline void trap(int y, int x)
    {
        if (in_safe(global_state.cur, y, x))
            trap_rows[y].set(x);
    }
    /**
     * 检查位置是否被阻挡
     */
    inline bool blocked(int y, int x) const { return in_safe(global_state.cur, y, x) ? blocked_rows[y].test(x) : true; }

    /**
     * 检查位置是否是敌方蛇身体
     */
    inline bool is_snake(int y, int x) const { return in_safe(global_state.cur, y, x) ? snake_rows[y].test(x) : false; }

    /**
     * 检查位置是否危险
     */
    inline bool is_danger(int y, int x) const { return in_safe(global_state.cur, y, x) ? danger_rows[y].test(x) : true; }

    /**
     * 检查位置是否为陷阱
     */
    inline bool is_trap(int y, int x) const { return in_safe(global_state.cur, y, x) ? trap_rows[y].test(x) : false; }
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

    // 1) 安全区域外 = 阻挡
    for (int y = 0; y < H; y++)
    {
        for (int x = 0; x < W; x++)
        {
            if (!in_safe(s.cur, y, x))
                M.block(y, x);
        }
    }

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
                if (in_safe(s.cur, p.y, p.x))
                    M.snake(p.y, p.x);
        }
    }

    // 3) 宝箱 = 阻挡障碍物 (如果自己没有钥匙)
    for (const auto &c : s.chests)
    {
        // 只有在没有钥匙的情况下，宝箱才视为障碍物
        if (!s.self().has_key)
        {
            if (in_safe(s.cur, c.pos.y, c.pos.x))
                M.block(c.pos.y, c.pos.x);
        }
    }

    // 4) 从物品中提取陷阱位置并标记（可以通过但不推荐）
    for (const auto &item : s.items)
    {
        if (item.type == -2) // 陷阱
        {
            if (in_safe(s.cur, item.pos.y, item.pos.x))
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

            if (in_safe(s.cur, ny, nx))
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

            // 检查边界和阻挡
            if (!in_safe(s.cur, ny, nx) || M.blocked(ny, nx))
                continue;

            int new_dist = out.dist[y][x] + 1;
            int new_snake_cost = out.snake_cost[y][x];

            // 如果是蛇身格子，增加蛇身代价
            if (M.is_snake(ny, nx))
            {
                // 如果没有护盾且无法激活护盾，给予很高的代价但不完全禁止
                if (s.snakes[s.self_idx].shield_time == 0 && !can_open_shield())
                {
                    new_snake_cost += SNAKE_COST_NO_SHIELD; // 高代价，但仍然可达
                }
                else if (s.snakes[s.self_idx].shield_time == 0 && can_open_shield()) // 如果可以使用护盾且现在没有护盾
                {
                    new_snake_cost += SNAKE_COST_OPEN_SHIELD; // 有护盾时的正常代价
                }
            }

            // 计算陷阱惩罚 - 软性避开陷阱但不完全禁止通过
            int extra_cost = 0;
            if (M.is_trap(ny, nx)) {
                extra_cost += TRAP_STEP_COST; // 对陷阱格子施加额外代价，使路径规划倾向于避开
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

    // 2) 以当前蛇头为起点做一次全图 BFS
    BFSOut G = bfs_grid(M, s, sy, sx);

    // 3) 小工具：判定 (y,x) 是否在 life 限制内可达
    auto reachable = [&](int y, int x, int life)
    {
        int d = (in_safe(s.cur, y, x) ? G.dist[y][x] : (int)1e9);
        int snake_steps = (in_safe(s.cur, y, x) ? G.snake_cost[y][x] : (int)1e9);
        if (d >= (int)1e9)
            return make_tuple(false, d, snake_steps);
        if (life != -1 && d > life)
            return make_tuple(false, d, snake_steps);
        return make_tuple(true, d, snake_steps);
    };

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
        auto [ok, d, snake_steps] = reachable(it.pos.y, it.pos.x, it.lifetime);
        if (!ok)
            continue;
        if (it.type == -2)
            continue;
        if (it.type == -5 && !me.has_key)
            continue;
        if (it.type == -3 && me.has_key)
            continue;

        double v = it.value;

        // 安全性惩罚：穿过蛇身的路径降低评分
        double safety_penalty = 1.0 + snake_steps * 0.5; // 每个蛇身格子降低50%的效率
        double sc = v / ((d + 1.0) * safety_penalty);
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
               << ",sc:" << (int)(sc * 100) << "|";
    }

    if (me.has_key)
    {
        log_ss << "CHEST_ANALYSIS:|";
        for (const auto &c : s.chests)
        {
            auto [ok, d, snake_steps] = reachable(c.pos.y, c.pos.x, -1);
            if (!ok)
                continue;
            double safety_penalty = 1.0 + snake_steps * 0.5;
            double sc = (c.score > 0 ? c.score * 2 : 60.0) / ((d + 1.0) * safety_penalty);
            cand.push_back({c.pos.y, c.pos.x, sc, d, snake_steps});

            log_ss << "CHEST@(" << c.pos.y << "," << c.pos.x
                   << ")d:" << d << ",s:" << snake_steps
                   << ",sc:" << (int)(sc * 100) << "|";
        }
    }

    log_ss << "CANDIDATES_COUNT:" << cand.size() << "|";

    auto last_choice = [&]()
    {
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

            // 检查该位置是否安全可移动：
            // - 必须在安全区域内
            // - 不能是阻挡位置（墙、陷阱、宝箱等）
            // - 不能是危险位置（敌蛇头附近）
            // - 不能是敌蛇身体
            // - 优先避免陷阱（在安全移动分析中）
            if (!in_safe(s.cur, ny, nx))
            {
                log_ss << ":UNSAFE|";
                continue;
            }
            if (M.blocked(ny, nx))
            {
                log_ss << ":BLOCKED|";
                continue;
            }
            if (M.is_danger(ny, nx))
            {
                log_ss << ":DANGEROUS|";
                continue;
            }
            if (M.is_snake(ny, nx))
            {
                log_ss << ":SNAKE_BODY|";
                continue;
            }
            // 在安全移动分析中避免陷阱 - 只有在所有其他选项都不安全时才会考虑陷阱
            if (M.is_trap(ny, nx)) {
                log_ss << ":TRAP|";
                continue;
            }

            // 计算该位置的"可达性"：统计从该位置能继续移动的方向数量
            int deg = 0; // 可达方向计数器
            for (int t = 0; t < 4; ++t)
            {
                int py = ny + DY[t], px = nx + DX[t]; // 从候选位置继续移动的位置
                // 检查是否为安全可移动的位置
                if (in_safe(s.cur, py, px) && !M.blocked(py, px) && !M.is_snake(py, px))
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
            str_info += log_ss.str();
            return ACT[bestDir];
        }

        // === 策略2：如果无安全移动，尝试开启护盾 ===
        // 条件：护盾冷却完毕且当前未开启护盾
        if (me.shield_cd == 0 && me.shield_time == 0)
        {
            log_ss << "SHIELD_ACTIVATION:|";
            str_info += log_ss.str();
            return 4; // 动作4 = 开启护盾
        }

        // === 策略3：最后的移动尝试（忽略危险性检查） ===
        // 当连护盾都开不了时，尝试任何可能的移动（即使有风险）
        log_ss << "DESPERATE_MOVE_ANALYSIS:|";
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

            // 只检查基本的边界和阻挡，忽略危险性
            if (in_safe(s.cur, ny, nx) && !M.blocked(ny, nx) && !M.is_snake(ny, nx))
            {
                log_ss << ":DESPERATE_MOVE_CHOSEN|";
                str_info += log_ss.str();
                return ACT[k];
            }
            else
            {
                log_ss << ":NOT_VIABLE|";
            }
        }

        // === 策略4：绝望的护盾尝试 ===
        // 如果连基本移动都不可能，强制尝试开启护盾（即使在冷却中）
        log_ss << "FORCED_SHIELD:|";
        str_info += log_ss.str();
        return 4;
    };

    // 5) 如果没有任何候选目标：执行“求生优先”兜底策略
    if (cand.empty())
    {
        int choice = last_choice();
        return {choice};
    }

    // 6) 选择评分最高的目标
    sort(cand.begin(), cand.end(), [](const Target &a, const Target &b)
         {
        if (fabs(a.score - b.score) > 1e-9) return a.score > b.score;
        return a.dist < b.dist; });
    const auto target = cand.front();

    log_ss << "TARGET_SELECTED:(" << target.y << "," << target.x
           << ")sc:" << (int)(target.score * 100) << ",d:" << target.dist
           << ",s:" << target.snake_cost << "|";

    // 7) 从目标位置回溯到蛇头，找到第一步应该走的方向
    int ty = target.y, tx = target.x; // 目标位置坐标

    // 检查目标位置是否可达（是否有父节点）
    if (G.parent[ty][tx] == -1)
    {
        log_ss << "ERROR:TARGET_UNREACHABLE|";
        str_info += log_ss.str();
        // 错误处理：如果护盾可用就开启，否则尝试向左移动
        int choice = last_choice();
        return {choice};
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
        log_ss << "ERROR:NO_DIRECTION|";
        str_info += log_ss.str();
        // 错误处理：优先开启护盾，否则默认向左移动
        int choice = last_choice();
        return {choice};
    }

    // === 防止180度掉头检查 ===
    // 蛇不能直接向相反方向移动（掉头）
    int opposite_dir = (me.dir + 2) % 4; // 计算上回合移动方向的相反方向
    if (dir == opposite_dir)
    {
        log_ss << "ERROR:REVERSE_DIRECTION_BLOCKED,prev_dir:" << me.dir << ",attempt_dir:" << dir << "|";
        str_info += log_ss.str();
        // 使用应急策略选择其他方向
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

    log_ss << "NEXT_STEP:" << next_direction << ",a:" << ACT[dir]
           << "@(" << cy << "," << cx << ")|";

    // 8) 移动前的安全性检查：危险位置检测
    // 如果下一步位置是危险区域（敌蛇头附近）且没有护盾保护
    if (M.is_danger(cy, cx) && me.shield_cd == 0 && me.shield_time == 0)
    {
        log_ss << "DANGER_DETECTED:|";
        str_info += log_ss.str();
        int choice = last_choice();
        return {choice};
    }

    // 9) 蛇身穿越检测：智能护盾使用策略
    if (M.is_snake(cy, cx)) // 如果下一步位置有敌方蛇身
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
        else if (me.shield_cd == 0) // 护盾冷却完毕
        {
            // 激活护盾为下一回合穿越蛇身做准备
            log_ss << "SHIELD_PREPARE:a:4|";
            str_info += log_ss.str();
            return {4}; // 开启护盾
        }
        // === 情况3：护盾不可用的绝望情况 ===
        else
        {
            log_ss << "SHIELD_UNAVAILABLE:|";
            str_info += log_ss.str();
            int choice = last_choice();
            return {choice};
        }
    }

    // 9.5) 陷阱检测：如果下一步是陷阱且有安全的替代方案，尝试避开
    if (M.is_trap(cy, cx)) {
        log_ss << "TRAP_NEXT_STEP:|";
        // 尝试寻找更安全的替代路径
        int choice = last_choice();
        // 注意：last_choice 中的绝望移动分析(DESPERATE_MOVE_ANALYSIS)仍然允许踩陷阱，
        // 这样当所有其他选项都比死亡更糟糕时，蛇仍然可以选择踩陷阱求生
        return {choice};
    }

    // 10) 正常移动：所有检查通过，执行计划的移动
    log_ss << "NORMAL_MOVE:" << next_direction << ",a:" << ACT[dir] << "|";
    str_info += log_ss.str();
    return {ACT[dir]}; // 返回对应的移动动作
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
