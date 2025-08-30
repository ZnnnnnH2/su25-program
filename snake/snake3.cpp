#include <bits/stdc++.h>
using namespace std;

// ==================== 游戏常量定义 ====================
static constexpr int W = 40;            // 地图宽度 (x坐标最大值)
static constexpr int H = 30;            // 地图高度 (y坐标最大值)
static constexpr int MYID = 2024201540; // TODO: 设置为你的学号

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
 * - -2: 陷阱（有害）
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
    int remaining_ticks;               // 游戏剩余回合数
    vector<Item> items;                // 地图上的所有物品
    vector<Snake> snakes;              // 游戏中的所有蛇
    vector<Chest> chests;              // 地图上的所有宝箱
    vector<Key> keys;                  // 地图上的所有钥匙
    vector<Point> traps;               // 地图上的所有陷阱
    Safe cur, next, fin;               // 当前、下次、最终安全区域
    int next_tick = -1, fin_tick = -1; // 安全区缩小的时间点
    int self_idx = -1;                 // 自己蛇在snakes数组中的索引

    // 获取自己蛇的便捷方法
    const Snake &self() const { return snakes[self_idx]; }
};

string str_info;

struct Worth
{
    bool ok; // allow shield?
    const char *why;
};

static Worth should_use_shield(const State &s, int target_value, bool survival_only = false)
{
    const auto &me = s.self();

    // forbid using shield when gain is small
    const int MIN_GAIN_TO_SHIELD = 15; // tunable; "too small" -> no shield
    if (!survival_only && target_value < MIN_GAIN_TO_SHIELD)
        return {false, "small"};

    // never if shield not ready
    if (me.shield_time > 0)
        return {true, "already_on"};
    if (me.shield_cd > 0)
        return {false, "cooling"};

    // additional late-game leniency (optional)
    if (!survival_only && s.remaining_ticks < 30)
        return {true, "late"};

    return {true, "ok"};
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
        exit(0);

    // ========== 读取物品信息 ==========
    int m;
    cin >> m;
    s.items.resize(m);
    for (int i = 0; i < m; i++)
    {
        cin >> s.items[i].pos.y >> s.items[i].pos.x >> s.items[i].type >> s.items[i].lifetime;
        switch (s.items[i].type)
        {
        case 5:
        case 4:
        case 3:
        case 2:
        case 1:
            s.items[i].value = s.items[i].type;
            break; // 普通食物
        case -1:
            s.items[i].value = 3;
            break; // 成长食物
        case -2:
            s.traps.push_back(s.items[i].pos);
            s.items[i].value = -2;
            break; // 陷阱
        case -3:
            s.items[i].value = 10;
            break; // 钥匙
        case -5:
            s.items[i].value = 10;
            break; // 宝箱
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
    cin >> str_info;
}

// ==================== 辅助函数 ====================

/**
 * 检查坐标是否在地图边界内
 */
inline bool in_bounds(int y, int x) { return (0 <= y && y < H && 0 <= x && x < W); }

/**
 * 检查坐标是否在安全区域内
 */
inline bool in_safe(const Safe &z, int y, int x)
{
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
    bitset<W> near_rows[H];    // NEW: cells adjacent to enemy bodies (soft danger)

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
        if (in_bounds(y, x))
            snake_rows[y].set(x);
    }

    /**
     * 标记位置为危险
     */
    inline void danger(int y, int x)
    {
        if (in_bounds(y, x))
            danger_rows[y].set(x);
    }

    /**
     * 标记位置为接近敌方蛇身体
     */
    inline void near(int y, int x)
    {
        if (in_bounds(y, x))
            near_rows[y].set(x);
    }

    /**
     * 检查位置是否被阻挡
     */
    inline bool blocked(int y, int x) const { return in_bounds(y, x) ? blocked_rows[y].test(x) : true; }

    /**
     * 检查位置是否是敌方蛇身体
     */
    inline bool is_snake(int y, int x) const { return in_bounds(y, x) ? snake_rows[y].test(x) : false; }

    /**
     * 检查位置是否危险
     */
    inline bool is_danger(int y, int x) const { return in_bounds(y, x) ? danger_rows[y].test(x) : true; }

    /**
     * 检查位置是否接近敌方蛇身体
     */
    inline bool is_near(int y, int x) const { return in_bounds(y, x) ? near_rows[y].test(x) : false; }
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
                if (in_bounds(p.y, p.x))
                {
                    M.snake(p.y, p.x); // remember enemy body
                    M.block(p.y, p.x); // hard wall
                }
        }
    }

    // 3) 宝箱 = 阻挡障碍物 (如果自己没有钥匙)
    for (const auto &c : s.chests)
    {
        // 只有在没有钥匙的情况下，宝箱才视为障碍物
        if (!s.self().has_key)
        {
            if (in_bounds(c.pos.y, c.pos.x))
                M.block(c.pos.y, c.pos.x);
        }
    }

    // 4) 陷阱 = 阻挡障碍物

    for (const auto &t : s.traps)
    {
        if (in_bounds(t.y, t.x))
            M.block(t.y, t.x);
    }

    // 5) 预测敌蛇头部邻居 = 危险位置
    for (const auto &sn : s.snakes)
    {
        if (sn.id == MYID) // 跳过自己的蛇
            continue;
        auto h = sn.head();
        if (in_bounds(h.y, h.x) && in_safe(s.cur, h.y, h.x))
            M.danger(h.y, h.x); // head itself
        // 标记敌蛇头部四周的位置为危险
        for (int k = 0; k < 4; k++)
        {
            int ny = h.y + DY[k], nx = h.x + DX[k];
            // 考虑敌蛇下一步可能移动到的位置
            // 如果我方蛇没有护盾，需要避开这些潜在的敌蛇头部位置

            if (in_bounds(ny, nx) && in_safe(s.cur, ny, nx))
                M.danger(ny, nx);
        }
    }

    // 6) NEW: body-adjacent halo (soft)
    for (int y = 0; y < H; y++)
        for (int x = 0; x < W; x++)
            if (M.is_snake(y, x))
            {
                for (int k = 0; k < 4; k++)
                {
                    int ny = y + DY[k], nx = x + DX[k];
                    if (in_bounds(ny, nx) && !M.blocked(ny, nx))
                        M.near(ny, nx);
                }
            }
    return M;
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

    // weights (tunable):
    const int W_BODY = 200; // cost per enemy-body cell traversed (very high)
    const int W_NEAR = 3;   // small bias to keep clearance
    const int W_STEP = 1;   // step cost

    using Node = tuple<int, int, int, int>; // (F, snake_cost, y, x)
    priority_queue<Node, vector<Node>, greater<>> pq;

    out.dist[sy][sx] = 0;
    pq.emplace(0, 0, sy, sx);

    auto f_of = [&](int d, int sc)
    { return d * W_STEP + sc * W_BODY; };

    while (!pq.empty())
    {
        auto [F, sc, y, x] = pq.top();
        pq.pop();
        // current best?
        if (F != f_of(out.dist[y][x], out.snake_cost[y][x]))
            continue;

        for (int k = 0; k < 4; k++)
        {
            int ny = y + DY[k], nx = x + DX[k];
            if (!in_bounds(ny, nx) || M.blocked(ny, nx))
                continue;

            int nd = out.dist[y][x] + 1;
            int nsc = out.snake_cost[y][x];

            // strong penalty for stepping on enemy body
            if (M.is_snake(ny, nx))
                nsc += 1;

            // soft penalty near enemy body
            int nF = f_of(nd, nsc) + (M.is_near(ny, nx) ? W_NEAR : 0);

            int curF = f_of(out.dist[ny][nx], out.snake_cost[ny][nx]);
            if (nF < curF)
            {
                out.dist[ny][nx] = nd;
                out.snake_cost[ny][nx] = nsc;
                out.parent[ny][nx] = (k + 2) % 4;
                pq.emplace(nF, nsc, ny, nx);
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
    log_ss << "|";

    // 2) 以当前蛇头为起点做一次全图 BFS
    BFSOut G = bfs_grid(M, s, sy, sx);

    // 3) 小工具：判定 (y,x) 是否在 life 限制内可达
    auto reachable = [&](int y, int x, int life)
    {
        int d = (in_bounds(y, x) ? G.dist[y][x] : (int)1e9);
        int snake_steps = (in_bounds(y, x) ? G.snake_cost[y][x] : (int)1e9);
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
        bool grows;
        int value; // NEW: raw value for shield policy
    };
    vector<Target> cand;
    cand.reserve(64);

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

        double v = 0.0;
        bool grows = false;
        if (it.type >= 1 && it.type <= 5)
            v = it.type * 2;
        else if (it.type == -1)
        {
            v = 10.0;
            grows = true;
        }
        else if (it.type == -3)
            v = 25.0;
        else if (it.type == -5)
            v = 100.0;

        // 安全性惩罚：穿过蛇身的路径降低评分
        double safety_penalty = 1.0 + snake_steps * 0.5; // 每个蛇身格子降低50%的效率
        double sc = v / ((d + 1.0) * safety_penalty);
        cand.push_back({it.pos.y, it.pos.x, sc, d, snake_steps, grows, (int)round(v)});
    }

    if (me.has_key)
    {
        for (const auto &c : s.chests)
        {
            auto [ok, d, snake_steps] = reachable(c.pos.y, c.pos.x, -1);
            if (!ok)
                continue;
            double safety_penalty = 1.0 + snake_steps * 0.5;
            double sc = (c.score > 0 ? c.score : 60.0) / ((d + 1.0) * safety_penalty);
            cand.push_back({c.pos.y, c.pos.x, sc, d, snake_steps, false, (c.score > 0 ? c.score : 60)});
        }
    }

    log_ss << "C" << cand.size();

    // 5) 如果没有任何候选目标：执行“求生优先”兜底策略
    if (cand.empty())
    {
        log_ss << "|RS"; // Reason: Survival
        int bestDir = -1;
        int bestReach = -1;
        for (int k = 0; k < 4; ++k)
        {
            int ny = sy + DY[k], nx = sx + DX[k];
            if (!in_bounds(ny, nx) || M.blocked(ny, nx) || M.is_danger(ny, nx) || M.is_snake(ny, nx))
                continue;

            int deg = 0;
            for (int t = 0; t < 4; ++t)
            {
                int py = ny + DY[t], px = nx + DX[t];
                if (in_bounds(py, px) && !M.blocked(py, px) && !M.is_snake(py, px))
                    ++deg;
            }
            if (deg > bestReach)
            {
                bestReach = deg;
                bestDir = k;
            }
        }

        if (bestDir != -1)
        {
            log_ss << "|M" << ACT[bestDir] << "|D" << bestReach;
            str_info += log_ss.str();
            return {ACT[bestDir]};
        }

        if (me.shield_cd == 0 && me.shield_time == 0)
        {
            log_ss << "|SF"; // Shield Fallback
            str_info += log_ss.str();
            return {4};
        }

        for (int k = 0; k < 4; ++k)
        {
            int ny = sy + DY[k], nx = sx + DX[k];
            if (in_bounds(ny, nx) && !M.blocked(ny, nx) && !M.is_snake(ny, nx))
            {
                log_ss << "|MF" << ACT[k]; // Move Fallback
                str_info += log_ss.str();
                return {ACT[k]};
            }
        }

        log_ss << "|SF"; // Final Shield Fallback
        str_info += log_ss.str();
        return {4};
    }

    // 6) 选择评分最高的目标
    sort(cand.begin(), cand.end(), [](const Target &a, const Target &b)
         {
        if (fabs(a.score - b.score) > 1e-9) return a.score > b.score;
        return a.dist < b.dist; });
    const auto target = cand.front();

    log_ss << "|RT|T" << target.y << "," << target.x << "|D" << target.dist << "|S" << target.snake_cost;

    // 7) 回溯路径
    int ty = target.y, tx = target.x;
    if (G.parent[ty][tx] == -1)
    {
        log_ss << "|ENP"; // Error: No Parent
        str_info += log_ss.str();
        if (me.shield_cd == 0 && me.shield_time == 0)
            return {4};
        return {0};
    }

    int cy = ty, cx = tx;
    while (!(cy == sy && cx == sx))
    {
        int back = G.parent[cy][cx];
        int py = cy + DY[back], px = cx + DX[back];
        if (py == sy && px == sx)
            break;
        cy = py;
        cx = px;
    }

    int dir = -1;
    for (int k = 0; k < 4; ++k)
        if (sy + DY[k] == cy && sx + DX[k] == cx)
        {
            dir = k;
            break;
        }

    if (dir == -1)
    {
        log_ss << "|END"; // Error: No Direction
        str_info += log_ss.str();
        if (me.shield_cd == 0 && me.shield_time == 0)
            return {4};
        return {0};
    }

    // --- Risk checks for the chosen first step (cy,cx) -> dir ---

    auto try_dodge = [&]() -> int
    {
        // choose the safest neighbor by: not blocked, not enemy body,
        // lowest danger, lowest near-penalty, many degrees
        int best = -1, bestScore = -1;
        for (int k = 0; k < 4; k++)
        {
            int ny = sy + DY[k], nx = sx + DX[k];
            if (!in_bounds(ny, nx) || M.blocked(ny, nx) || M.is_snake(ny, nx))
                continue;
            // prefer non-danger, non-near, higher free-degree
            int deg = 0;
            for (int t = 0; t < 4; t++)
            {
                int py = ny + DY[t], px = nx + DX[t];
                if (in_bounds(py, px) && !M.blocked(py, px) && !M.is_snake(py, px))
                    deg++;
            }
            int score = (M.is_danger(ny, nx) ? 0 : 2) + (M.is_near(ny, nx) ? 0 : 1) + deg;
            if (score > bestScore)
            {
                bestScore = score;
                best = k;
            }
        }
        return best; // -1 if none
    };

    // 8) hard risk: enemy-head zone or enemy body
    bool step_danger = M.is_danger(cy, cx);
    bool step_body = M.is_snake(cy, cx);

    if (step_body || step_danger)
    {
        // Try to dodge first
        int alt = try_dodge();
        if (alt != -1)
        {
            log_ss << "|DJ" << ACT[alt]; // log: Dodged
            str_info += log_ss.str();
            return {ACT[alt]};
        }

        // If no dodge, consider shield policy:
        // - If stepping into BODY: only allow if survival or big gain.
        // - If stepping into DANGER (head fight): same rule.
        auto gate = should_use_shield(s, target.value, /*survival_only=*/false);
        if (!gate.ok)
        {
            // As we are forced, attempt any legal non-body move (even if danger)
            for (int k = 0; k < 4; k++)
            {
                int ny = sy + DY[k], nx = sx + DX[k];
                if (in_bounds(ny, nx) && !M.blocked(ny, nx) && !M.is_snake(ny, nx))
                {
                    log_ss << "|F" << ACT[k]; // Forced move
                    str_info += log_ss.str();
                    return {ACT[k]};
                }
            }
            // survival-only: if literally no other output, allow shield if ready
            gate = should_use_shield(s, /*tiny*/ 0, /*survival_only=*/true);
            if (gate.ok)
            {
                log_ss << "|SF"; // Survival shield
                str_info += log_ss.str();
                return {4};
            }
            // otherwise just pick dir (may die, but policy respected)
            log_ss << "|M" << ACT[dir];
            str_info += log_ss.str();
            return {ACT[dir]};
        }
        else
        {
            // Shield allowed (big gain or late game) — but only if shield is actually available
            const auto &me = s.self();
            if (me.shield_time > 0 || me.shield_cd == 0)
            {
                log_ss << "|SS"; // Shield for big value
                str_info += log_ss.str();
                return {4};
            }
            // else: do our best without shield
            int alt2 = try_dodge();
            if (alt2 != -1)
            {
                log_ss << "|DJ2" << ACT[alt2]; // Second dodge attempt
                str_info += log_ss.str();
                return {ACT[alt2]};
            }
            log_ss << "|M" << ACT[dir];
            str_info += log_ss.str();
            return {ACT[dir]};
        }
    }

    // 9) normal safe case
    log_ss << "|M" << ACT[dir];
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

    State s;
    read_state(s);
    auto choice = decide(s);
    cout << choice.action << "\n";
    cout << str_info << "\n";
    return 0;
}
