Paste-in fixed code (canonical versions)
1) Keep this BFSOut + bfs_grid and delete all other BFS variants1） 保留此 BFSOut + bfs_grid 并删除所有其他 BFS 变体

Use the array + int parent design because your path backtracking uses directions (see G2.parent[cy][cx] as an int). This matches your current reconstruction logic. (The version below is the “good” one I found in your file, cleaned and ready.)使用数组 + int 父设计，因为您的路径回溯使用方向（参见 G2.parent[cy][cx] 作为 int）。这与您当前的重建逻辑相匹配。（下面的版本是我在你的文件中找到的“好”版本，已经清理干净并准备就绪。

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

// Dijkstra on grid with soft penalties; forbids U-turn only on the first step
BFSOut bfs_grid(const GridMask &M, const State &s, int sy, int sx,
                const Snake *snake_for_pathfinding = nullptr)
{
    BFSOut G;

    const Snake &path_snake = snake_for_pathfinding ? *snake_for_pathfinding : s.self();
    const int opposite_dir = (path_snake.dir + 2) % 4;

    using Node = tuple<int,int,int>; // (cost, y, x)
    priority_queue<Node, vector<Node>, greater<Node>> pq;

    auto push = [&](int d, int y, int x, int from_dir)
    {
        if (d < G.dist[y][x]) {
            G.dist[y][x] = d;
            G.parent[y][x] = from_dir; // dir from parent -> (y,x)
            pq.emplace(d, y, x);
        }
    };

    G.dist[sy][sx] = 0;
    pq.emplace(0, sy, sx);

    while (!pq.empty()) {
        auto [cd, cy, cx] = pq.top();
        pq.pop();
        if (cd != G.dist[cy][cx]) continue; // stale

        for (int k = 0; k < 4; ++k) {
            int ny = cy + DY[k], nx = cx + DX[k];
            if (!in_bounds(ny, nx)) continue;
            if (!in_safe_zone(s.cur, ny, nx)) continue; // outside safe zone blocked
            if (M.blocked(ny, nx)) continue;

            // forbid U-turn only for the very first step from the source
            if (cy == sy && cx == sx && k == opposite_dir) continue;

            int step = 1;
            if (M.is_trap(ny, nx)) step += TRAP_STEP_COST;

            bool near_snake = false;
            for (int t = 0; t < 4; ++t) {
                int ay = ny + DY[t], ax = nx + DX[t];
                if (in_bounds(ay, ax) && M.is_snake(ay, ax)) { near_snake = true; break; }
            }
            if (near_snake) step += NEAR_ENEMY_ADJ_PENALTY;

            int nd = cd + step;
            push(nd, ny, nx, k);
        }
    }
    return G;
}


Your existing backtracking that does:您现有的回溯：

while (G2.dist[cy][cx] > 1) {
    int parent_dir = G2.parent[cy][cx];
    int parent_y = cy - DY[parent_dir], parent_x = cx - DX[parent_dir];
    cy = parent_y; cx = parent_x;
}


will work with this BFS (it assumes parent is a direction). 将与此 BFS 一起使用（它假设父是一个方向）。

Delete the other BFS variant that defines删除定义
struct BFSOut { vector<vector<int>> dist; vector<vector<Point>> parent; }
and its bfs_grid—it doesn’t match your call sites. 它的 bfs_grid——它与您的通话站点不匹配。

2) Fix the danger_safe_zone wrapper2）固定 danger_safe_zone 包装纸

Keep your full signature version, but the simple wrapper must not contain dead code after the return. Use this exact wrapper and delete the unreachable block currently below it. 保留完整的签名版本，但简单包装器在返回后不得包含死代码。使用这个确切的包装器并删除其下方当前无法访问的块 。

// Canonical hazard evaluator (keep your long version above)
// ...

// Compatibility wrapper – DO NOT put anything after the return.
inline int danger_safe_zone(const Safe &z, int y, int x)
{
    return danger_safe_zone(z, global_state.next, global_state.next_tick,
                            global_state.current_ticks, y, x);
}