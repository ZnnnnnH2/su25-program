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
                    M.snake(p.y, p.x);
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
        // 标记敌蛇头部四周的位置为危险
        for (int k = 0; k < 4; k++)
        {
            int ny = h.y + DY[k], nx = h.x + DX[k];
            // 如果敌蛇持有护盾，其头部不会造成威胁，可以穿过。但如果敌蛇没有护盾，我们仍然要避免与它碰撞
            // 考虑敌蛇下一步可能移动到的位置
            // 如果我方蛇没有护盾，需要避开这些潜在的敌蛇头部位置

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
    array<array<int, W>, H> dist;   // 距离矩阵 dist[y][x]
    array<array<int, W>, H> parent; // 父节点方向 parent[y][x]
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
            out.parent[y][x] = -1;
        }

    deque<pair<int, int>> dq; // BFS队列

    // 起始位置不被阻挡，加入队列
    out.dist[sy][sx] = 0;
    dq.emplace_back(sy, sx);

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

            // 检查边界和阻挡，有盾且时间足够就上盾
            if (!in_bounds(ny, nx) || M.blocked(ny, nx) || (M.is_snake(ny, nx) && s.snakes[s.self_idx].shield_time <= dcur + 1))
                continue;

            // 如果找到更短路径，更新距离和父节点
            if (out.dist[ny][nx] > dcur + 1)
            {
                out.dist[ny][nx] = dcur + 1;
                // parent 存储的是从 (ny, nx) 返回 (y, x) 的方向，即当前移动的逆方向
                out.parent[ny][nx] = (k + 2) % 4;
                dq.emplace_back(ny, nx); // 加入队列
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

    // 2) 以当前蛇头为起点做一次全图 BFS，得到到所有可达点的最短步数与回溯父方向
    BFSOut G = bfs_grid(M, s, sy, sx);

    // 3) 小工具：判定 (y,x) 是否在 life 限制内可达；返回 (可达标志, 最短距离)
    // life = -1 表示忽略寿命 / 剩余时间限制；否则要求距离 <= life
    // BFS 未触达的格子 dist 为 1e9 视为不可达
    // 注意：这里不额外判断危险格，危险只在真正落子时做硬性规避 / 护盾策略
    /**
     * reachable: 判断坐标(y,x)是否在 BFS 图上可达且满足寿命限制
     *  - d == 1e9 => 不可达
     *  - life != -1 且 d > life => 超过存活/存在时间限制（例如物品剩余寿命）
     */
    auto reachable = [&](int y, int x, int life)
    {
        int d = (in_bounds(y, x) ? G.dist[y][x] : (int)1e9);
        if (d >= (int)1e9)
            return make_pair(false, d);
        if (life != -1 && d > life)
            return make_pair(false, d);
        return make_pair(true, d);
    };

    struct Target
    {
        int y, x;
        double score;
        int dist;
        bool grows;
    };
    vector<Target> cand;
    cand.reserve(64);

    // 4) 枚举地图上可作为目标的物品，构建候选列表
    //    当前策略：基于 (价值 / (距离+1)) 作为启发式评分；陷阱直接跳过
    //    钥匙：仅当地图上存在宝箱且自己尚无钥匙时才追求
    //    宝箱：仅在已经持有钥匙时才加入候选（否则当作阻挡在 mask 中已处理）
    //    成长食物：给予固定提升 (这里设为 5.0) 并标记 grows 标志，后续避免踩尾时使用
    //    注意：尚未加入空间评估 / 风险折扣 / 多回合预测，可作为下一步优化
    // 2.1 normal items
    for (const auto &it : s.items)
    {
        auto [ok, d] = reachable(it.pos.y, it.pos.x, it.lifetime);
        if (!ok)
            continue;

        // Skip traps
        if (it.type == -2)
            continue;

        // Chest logic: only pursue if we already have a key
        if (it.type == -5 && !me.has_key)
            continue;

        // Key logic: only pursue if we don't have key //ignore and there is a chest
        // bool haveChest = !s.chests.empty();
        if (it.type == -3 && me.has_key) //(me.has_key || !haveChest)
            continue;

        double v = 0.0;
        bool grows = false;
        if (it.type >= 1 && it.type <= 5) // normal food
            v = it.type * 2;              // normal food
        else if (it.type == -1)
        {
            v = 10.0;
            grows = true;
        } // growth bean
        else if (it.type == -3)
            v = 25.0; // key (if chest exists)
        else if (it.type == -5)
            v = 100.0; // chest (we have key) TODO: if chest have high grades i will up.it's value

        // primary: value density; softer space/center terms removed here
        double sc = v / (d + 1.0);
        cand.push_back({it.pos.y, it.pos.x, sc, d, grows});
    }

    // 2.2 add explicit chests from s.chests when we hold a key
    if (me.has_key)
    {
        for (const auto &c : s.chests)
        {
            auto [ok, d] = reachable(c.pos.y, c.pos.x, -1);
            if (!ok)
                continue;
            double sc = (c.score > 0 ? c.score : 60.0) / (d + 1.0); // high value
            cand.push_back({c.pos.y, c.pos.x, sc, d, /*grows*/ false});
        }
    }
    str_info += to_string(cand.size()) + " ";
    // 5) 如果没有任何候选目标：执行“求生优先”兜底策略
    //    逻辑：在四个方向里找一个非阻挡且非危险的格子，优先局部可继续展开的（统计其周围可走邻居数量）
    //    若无安全步并且护盾可用 -> 开盾
    //    若依然无路 -> 选择任意能走的一步（可能是撞击 / 冒险）或最终开盾
    // No good targets? fall back to safest non-danger move
    if (cand.empty())
    {
        // ================= 兜底：无任何可行目标 (无食物 / 都不可达) 时的“求生”策略 =================
        // 思路：在四个基本方向中挑选：安全 + 留有更多后续扩展空间 的那一步。
        // 步骤：
        //   1) 枚举四个方向（左上右下） -> (ny,nx) 为潜在下一格。
        //   2) 过滤：越界 / 被阻挡 / 预测危险(敌头周围) 直接放弃。
        //   3) 对保留下来的合法候选，计算其“局部可扩展度” deg：统计该格四邻中仍可走(未阻挡)的数量。
        //      这是一个很粗糙但常见的局部空域启发(局部度数 / local degree)，避免钻入死胡同。
        //   4) 取 deg 最大的方向；若多个方向同 deg，保持第一个（可改进为随机化防止模式化）。
        int bestDir = -1;   // 记录当前最佳方向索引 k (0..3)
        int bestReach = -1; // 记录该方向对应的局部可扩展度 (度数更大优先)
        for (int k = 0; k < 4; ++k)
        {
            int ny = sy + DY[k], nx = sx + DX[k]; // 计算下一步格子坐标
            // 过滤条件：
            //  - 越界 -> 不能走
            //  - 阻挡 -> 身体/墙/安全区外/宝箱(未拿钥匙)/陷阱 已在 mask 中标记
            //  - 危险 -> 敌方蛇头下一步可能到达区域，优先回避
            if (!in_bounds(ny, nx) || M.blocked(ny, nx) || M.is_danger(ny, nx) || M.is_snake(ny, nx))
                continue;
            // 初步认为 k 是一个候选方向（即便后面可能被更高 deg 取代）
            bestDir = k;
            // 计算局部度数：统计 (ny,nx) 的 4 邻里中未被阻挡的数量，用作简单“空间”评估
            int deg = 0;
            for (int t = 0; t < 4; ++t)
            {
                int py = ny + DY[t], px = nx + DX[t];
                if (in_bounds(py, px) && !M.blocked(py, px))
                    ++deg;
            }
            // 如果该方向拥有更高的局部自由度，则更新最佳选择
            if (deg > bestReach)
            {
                bestReach = deg;
                bestDir = k;
            }
        }
        // 找到了至少一个安全方向 -> 直接返回该方向动作
        if (bestDir != -1)
            return {ACT[bestDir]};
        // 没有任何安全方向：说明四邻都阻挡或危险；若护盾冷却为 0 且当前未在护盾 -> 尝试开盾苟活
        if (me.shield_cd == 0 && me.shield_time == 0)
            return {4};
        // 仍然没有盾 / 不想开盾：最后再尝试“硬闯”一个未阻挡（即使是危险）的格子，尽量不原地等死
        for (int k = 0; k < 4; ++k)
        {
            int ny = sy + DY[k], nx = sx + DX[k];
            if (in_bounds(ny, nx) && !M.blocked(ny, nx))
                return {ACT[k]}; // 可能是危险格，但比完全无移动更好（可视对局规则调整）
        }
        // 所有方向都被阻挡（包括撞身体 / 墙 / 区域外），只能寄希望护盾（若前面未触发，这里兜底返回 4）
        return {4};
    }

    // 6) 选择评分最高的目标（分数优先, 若近似相等再按距离更短）
    //    cand.front() 排序后即为最佳目标
    // choose best target
    sort(cand.begin(), cand.end(), [](const Target &a, const Target &b)
         {
        if (fabs(a.score - b.score) > 1e-9) return a.score > b.score;
        return a.dist < b.dist; });
    const auto target = cand.front();

    // 7) 回溯路径：从目标格反向沿 parent 走到与起点相邻的第一步格 (cy,cx)
    //    parent[y][x] 存储的是“从 (y,x) 回到其父格的方向”，因此回溯时不断朝 parent 指向的方向前进
    // reconstruct first step from parent grid
    int ty = target.y, tx = target.x;
    if (G.parent[ty][tx] == -1)
    {
        // shouldn't happen (we checked reachable), be safe:
        if (me.shield_cd == 0 && me.shield_time == 0)
            return {4};
        return {0};
        str_info += "no parent";
    }
    // walk back from target to the neighbor of (sy,sx)
    int cy = ty, cx = tx;
    while (!(cy == sy && cx == sx))
    {
        int back = G.parent[cy][cx]; // direction to go back toward parent
        int py = cy + DY[back], px = cx + DX[back];
        if (py == sy && px == sx)
            break;
        cy = py;
        cx = px;
    }
    // (cy,cx) is the first step
    int dir = -1;
    for (int k = 0; k < 4; ++k)
        if (sy + DY[k] == cy && sx + DX[k] == cx)
        {
            dir = k;
            break;
        }
    if (dir == -1)
    {
        if (me.shield_cd == 0 && me.shield_time == 0)
            return {4};
        return {0};
    }

    // 8) 危险格硬检测：若第一步落在敌头下一步可达预测区域且可立即开盾 -> 用护盾替代移动
    //    （也可改为：若危险但收益高时赌一手；当前实现保守）
    // danger hard check: if first step lands in predicted enemy-head zone and shield ready, use shield
    if (M.is_danger(cy, cx) && me.shield_cd == 0 && me.shield_time == 0)
        return {4};

    // 9)（已简化）根据游戏规则：自身身体（含尾巴所在格）本回合不会造成自撞
    //    原先为“成长导致尾巴不前进需避免踩尾”的逻辑已移除；不再区分增长与否，直接执行路径第一步。
    //    如果未来规则改变（例如身体立即判撞），可恢复此前 growsNow 检测与尾巴占用回避代码。

    // 10) 返回最终决策动作（0~3 对应 左上右下）。未触发护盾兜底则直接执行第一步方向
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
    return 0;
}
