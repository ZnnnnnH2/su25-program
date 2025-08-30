/**
 * 高级贪吃蛇AI - 滚动地平线规划器 (≤0.7s 每回合)
 *
 * 算法特点：
 * - 每回合重新构建BFS从蛇头到所有可达目标
 * - 将其他蛇视为硬障碍物
 * - 将宝箱视为障碍物（阻挡格子）
 * - 暂时忽略钥匙和宝箱作为目标（不计分）
 * - 输出一个移动指令 (0=左,1=上,2=右,3=下, 4=护盾)
 *
 * 坐标系统：输入格式为 (y x)。网格：宽度=40 (x ∈ [0..39])，高度=30 (y ∈ [0..29])
 */

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
    int value;    // 物品价值/类型
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
    Safe cur, next, fin;               // 当前、下次、最终安全区域
    int next_tick = -1, fin_tick = -1; // 安全区缩小的时间点
    int self_idx = -1;                 // 自己蛇在snakes数组中的索引

    // 获取自己蛇的便捷方法
    const Snake &self() const { return snakes[self_idx]; }
};

// ==================== 输入输出：每回合读取游戏状态 ====================

/**
 * 从标准输入读取游戏状态（API格式）
 * 优化了输入性能
 */
static void read_state(State &s)
{
    // 优化输入性能
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    // 读取剩余回合数，如果读取失败则退出
    if (!(cin >> s.remaining_ticks))
        exit(0);

    // ========== 读取物品信息 ==========
    int m;
    cin >> m;
    s.items.resize(m);
    for (int i = 0; i < m; i++)
    {
        cin >> s.items[i].pos.y >> s.items[i].pos.x >> s.items[i].value >> s.items[i].lifetime;
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
            s.self_idx = i;
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
    bitset<W> blocked_rows[H]; // 阻挡位置的位掩码
    bitset<W> danger_rows[H];  // 危险位置的位掩码

    /**
     * 标记位置为阻挡
     */
    inline void block(int y, int x)
    {
        if (in_bounds(y, x))
            blocked_rows[y].set(x);
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
     * 检查位置是否被阻挡
     */
    inline bool blocked(int y, int x) const { return in_bounds(y, x) ? blocked_rows[y].test(x) : true; }

    /**
     * 检查位置是否危险
     */
    inline bool is_danger(int y, int x) const { return in_bounds(y, x) ? danger_rows[y].test(x) : true; }
};

// ==================== 地图掩码构建 ====================

/**
 * 为当前回合构建阻挡和危险位置掩码
 *
 * 阻挡位置包括：
 * 1. 安全区域外的位置
 * 2. 其他蛇的身体
 * 3. 宝箱位置
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

    // 2) 所有蛇的身体 = 阻挡（包括自己的蛇）
    for (const auto &sn : s.snakes)
    {
        for (const auto &p : sn.body)
            if (in_bounds(p.y, p.x))
                M.block(p.y, p.x);
    }

    // 3) 宝箱 = 阻挡障碍物
    for (const auto &c : s.chests)
    {
        if (in_bounds(c.pos.y, c.pos.x))
            M.block(c.pos.y, c.pos.x);
    }

    // 4) 预测敌蛇头部邻居 = 危险位置
    for (const auto &sn : s.snakes)
    {
        if (sn.id == MYID) // 跳过自己的蛇
            continue;
        auto h = sn.head();
        // 标记敌蛇头部四周的位置为危险
        for (int k = 0; k < 4; k++)
        {
            int ny = h.y + DY[k], nx = h.x + DX[k];
            if (in_bounds(ny, nx) && in_safe(s.cur, ny, nx))
                M.danger(ny, nx);
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
    array<array<int, W>, H> dist;   // 距离矩阵 dist[y][x] - 修复坐标顺序
    array<array<int, W>, H> parent; // 父节点方向 parent[y][x] - 修复坐标顺序
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
static BFSOut bfs_grid(const GridMask &M, int sy, int sx)
{
    BFSOut out;

    // 初始化距离为无穷大，父节点为-1
    for (int y = 0; y < H; y++)
        for (int x = 0; x < W; x++)
        {
            out.dist[y][x] = 1e9;
            out.parent[y][x] = -1;
        }

    deque<pair<int, int>> dq; // BFS队列

    // 如果起始位置不被阻挡，加入队列
    if (!M.blocked(sy, sx))
    {
        out.dist[sy][sx] = 0;
        dq.emplace_back(sy, sx);
    }

    // BFS主循环
    while (!dq.empty())
    {
        auto [y, x] = dq.front();
        dq.pop_front();
        int dcur = out.dist[y][x];

        // 尝试四个方向
        for (int k = 0; k < 4; k++)
        {
            int ny = y + DY[k], nx = x + DX[k];

            // 检查边界和阻挡
            if (!in_bounds(ny, nx) || M.blocked(ny, nx))
                continue;

            // 如果找到更短路径，更新距离和父节点
            if (out.dist[ny][nx] > dcur + 1)
            {
                out.dist[ny][nx] = dcur + 1;
                out.parent[ny][nx] = (k + 2) % 4; // 修复方向计算：记录反方向
                dq.emplace_back(ny, nx);
            }
        }
    }
    return out;
}

// ==================== 决策算法：只针对食物/成长物品 ====================

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
    GridMask M = build_masks(s);
    int sy = me.head().y, sx = me.head().x;
    auto G = bfs_grid(M, sy, sx);

    // ========== 物品评分函数 ==========
    /**
     * 为物品计算价值分数
     * 考虑因素：物品价值、距离、生存时间、方向偏好
     */
    auto score_item = [&](const Item &it) -> double
    {
        // 忽略钥匙、宝箱和陷阱
        if (it.value == -3 || it.value == -5) // 钥匙、宝箱
            return -1e9;
        if (it.value == -2) // 陷阱
            return -1e9;

        int d = G.dist[it.pos.x][it.pos.y]; // 修复坐标顺序

        // 无法到达
        if (d >= 1e9)
            return -1e9;

        // 到达时物品已经消失
        if (it.lifetime != -1 && d > it.lifetime)
            return -1e9;

        double v = 0.0;
        if (it.value >= 1) // 普通食物
            v = it.value;
        else if (it.value == -1) // 成长食物
            v = 5.0;             // 成长食物的价值提高
        else
            return -1e9;

        // 方向偏好：优先选择与当前方向一致的目标
        double direction_bonus = 0.0;
        int target_dir = -1;
        if (it.pos.x < sx)
            target_dir = 0; // 左
        else if (it.pos.y < sy)
            target_dir = 1; // 上
        else if (it.pos.x > sx)
            target_dir = 2; // 右
        else if (it.pos.y > sy)
            target_dir = 3; // 下

        if (target_dir == me.dir)
            direction_bonus = 3.0; // 增加方向奖励

        // 距离奖励：优先选择较近的高价值食物
        double distance_bonus = max(0.0, 10.0 - d);

        // 价值密度：价值除以（距离+1）+ 方向奖励 + 距离奖励
        return (v + direction_bonus + distance_bonus) / (d + 1.0);
    };

    // ========== 目标候选结构 ==========
    struct Target
    {
        Point p;   // 目标位置
        double sc; // 评分
        int dist;  // 距离
    };

    // 构建候选目标列表
    vector<Target> cand;
    // ========== 新策略：对每个方向进行前瞻评估 (Lookahead) ==========
    // 目的：提升主动寻食效率，同时兼顾安全（空间、危险格避让）

    // 快速函数：给定距离返回价值密度分数（与 score_item 一致）
    auto value_density_with_dist = [&](int val, int distSteps, int lifetime) -> double
    {
        if (distSteps >= 1e9)
            return -1e9;
        if (lifetime != -1 && distSteps > lifetime)
            return -1e9;
        double v = 0.0;
        if (val >= 1)
            v = val; // 普通食物
        else if (val == -1)
            v = 3.0; // 成长食物
        else
            return -1e9; // 其它忽略
        return v / (distSteps + 1.0);
    };

    struct MoveEval
    {
        int dir;
        double comp;
        double bestFood;
        int foodDist;
        int space;
        bool danger;
        bool valid;
    };
    vector<MoveEval> evals;
    evals.reserve(4);

    // 计算安全区中心（用于微弱引导保持居中，防止被圈）
    int cx = (s.cur.x_min + s.cur.x_max) / 2;
    int cy = (s.cur.y_min + s.cur.y_max) / 2;

    for (int k = 0; k < 4; ++k)
    {
        int ny = sy + DY[k], nx = sx + DX[k];
        if (!in_bounds(ny, nx) || M.blocked(ny, nx))
            continue; // 不可行

        // 前瞻：从该下一步位置重新 BFS
        auto G2 = bfs_grid(M, ny, nx);
        double bestFood = -1e9;
        int bestFoodDist = 1e9;
        for (const auto &it : s.items)
        {
            // 仅对有效食物计算（与 score_item 逻辑一致）
            if (it.value == -3 || it.value == -5 || it.value == -2)
                continue; // 忽略钥匙/宝箱/陷阱
            int d2 = G2.dist[it.pos.x][it.pos.y];
            double sc2 = value_density_with_dist(it.value, d2, it.lifetime);
            if (sc2 > bestFood || (fabs(sc2 - bestFood) < 1e-9 && d2 < bestFoodDist))
            {
                bestFood = sc2;
                bestFoodDist = d2;
            }
        }
        if (bestFood < -1e8)
        {
            bestFood = 0.0;
            bestFoodDist = 1e9;
        }

        // 统计可达空间规模（可走格子的数量，用于评估生存空间）
        int space = 0;
        for (int xx = 0; xx < W; ++xx)
            for (int yy = 0; yy < H; ++yy)
                if (G2.dist[xx][yy] < 1e9)
                    ++space;

        bool dangerNext = M.is_danger(ny, nx);

        // 组合评分：
        // 食物分  * 100  （主导）
        // 空间规模 * 0.4  （避免自陷）
        // 距离中心的负值 * 0.1 （鼓励靠近中心）
        // 危险格惩罚：-60
        double comp = 0.0;
        comp += bestFood * 100.0;
        comp += space * 0.4;
        comp -= (abs(nx - cx) + abs(ny - cy)) * 0.1;
        if (dangerNext)
            comp -= 60.0;

        // 如果没有任何食物且空间很小，降低权重（防止盲目钻死角）
        if (bestFood == 0.0 && space < 80)
            comp -= 30.0;

        evals.push_back({k, comp, bestFood, bestFoodDist, space, dangerNext});
    }

    if (!evals.empty())
    {
        // 排序选择最优动作：按综合分降序；如分数接近（<5差距），优先非危险、空间大
        sort(evals.begin(), evals.end(), [](const MoveEval &a, const MoveEval &b)
             { return a.comp > b.comp; });

        // 若最佳是危险格且有非危险且分数差距不大，换成安全
        if (evals[0].danger)
        {
            for (size_t i = 1; i < evals.size(); ++i)
            {
                if (!evals[i].danger && evals[0].comp - evals[i].comp < 25.0)
                    return {ACT[evals[i].dir]};
            }
            if (me.shield_cd == 0 && me.shield_time == 0)
                return {4};
        }
        return {ACT[evals[0].dir]};
    }
    // 后备策略（四个方向都不可行）
    int cx0 = (s.cur.x_min + s.cur.x_max) / 2;
    int cy0 = (s.cur.y_min + s.cur.y_max) / 2;
    int bestAct = -1, bestScore = -1e9;
    for (int k = 0; k < 4; ++k)
    {
        int ny = sy + DY[k], nx = sx + DX[k];
        if (!in_bounds(ny, nx) || M.blocked(ny, nx))
            continue;
        int scr = 0;
        if (!M.is_danger(ny, nx))
            scr += 5;
        scr -= abs(nx - cx0) + abs(ny - cy0);
        if (scr > bestScore)
        {
            bestScore = scr;
            bestAct = ACT[k];
        }
    }
    if (bestAct == -1 && me.shield_cd == 0)
        return {4};
    if (bestAct == -1)
    {
        for (int k = 0; k < 4; ++k)
        {
            int ny = sy + DY[k], nx = sx + DX[k];
            if (in_bounds(ny, nx) && !M.blocked(ny, nx))
                return {ACT[k]};
        }
        return {4};
    }
    return {bestAct};
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
    State s;
    read_state(s);                 // 读取当前游戏状态
    auto choice = decide(s);       // 执行决策算法
    cout << choice.action << "\n"; // 输出选择的动作
    return 0;
}
