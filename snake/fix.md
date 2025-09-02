Got it — you want me to **fix the BFS movement + backtracking**, **wire a true one-step extraction from the path**, **correct the chest value**, and **make the I/O (memory read/write) 100% API-compliant**. Below is a compact, drop-in patch you can apply to your current `snake-fused.cpp`. I’m giving you only the parts you need to **rewrite**, not the whole file.

---

# 1) Replace your BFS output + backtracking with a parent chain

### **Add/replace these types and helpers (once)**

Put them near your other pathfinding utilities.

```cpp
// ---------- BFS output with parent pointers ----------
struct BFSOut {
    vector<vector<int>> dist;        // H x W
    vector<vector<Point>> parent;    // H x W, parent[y][x] = previous cell on shortest path
};

// Safe sentinel
static inline bool is_nil_parent(const Point& p) { return p.y < 0 || p.x < 0; }

// Reconstruct first step direction from (sy,sx) to (ty,tx)
// returns -1 if unreachable or target==start
static int reconstruct_first_step_dir(const BFSOut& G, int sy, int sx, int ty, int tx) {
    if (sy == ty && sx == tx) return -1;                 // already there
    if (G.dist[ty][tx] >= INF) return -1;                // unreachable

    // Walk back from target to start
    Point cur{ty, tx};
    Point prev = G.parent[cur.y][cur.x];
    if (is_nil_parent(prev)) return -1;

    // Climb parent chain until we reach the immediate neighbor of (sy,sx)
    while (!(prev.y == sy && prev.x == sx)) {
        cur = prev;
        prev = G.parent[cur.y][cur.x];
        if (is_nil_parent(prev)) return -1; // broken chain (shouldn't happen if dist valid)
    }

    // cur is the first step cell
    int dy = cur.y - sy, dx = cur.x - sx;
    if (dy == 0 && dx == -1) return 0; // LEFT
    if (dy == -1 && dx == 0) return 1; // UP
    if (dy == 0 && dx == 1) return 2;  // RIGHT
    if (dy == 1 && dx == 0) return 3;  // DOWN
    return -1;
}
```

### **Rewrite your `bfs_grid` to fill `parent`**

Anywhere you define/return `BFSOut`, initialize and assign parents when you relax a cell:

```cpp
static BFSOut bfs_grid(const GridMask& M, const State& s, int sy, int sx) {
    BFSOut out;
    out.dist.assign(H, vector<int>(W, INF));
    out.parent.assign(H, vector<Point>(W, Point{-1,-1}));

    deque<Point> q;
    out.dist[sy][sx] = 0;
    out.parent[sy][sx] = Point{-1,-1}; // start has no parent
    q.push_back(Point{sy, sx});

    while (!q.empty()) {
        Point u = q.front(); q.pop_front();
        int du = out.dist[u.y][u.x];

        for (int k = 0; k < 4; ++k) {
            int ny = u.y + DY[k], nx = u.x + DX[k];
            if (!in_bounds(ny, nx)) continue;
            if (M.blocked(ny, nx)) continue;   // your existing mask checks
            if (du + 1 < out.dist[ny][nx]) {
                out.dist[ny][nx] = du + 1;
                out.parent[ny][nx] = u;       // <-- record parent
                q.push_back(Point{ny, nx});
            }
        }
    }
    return out;
}
```

> Why: your current backtracking code walks by comparing `dist` differences and sometimes “slides” diagonally in tie cases or stops at `dist>1` and guesses the step. Using a **parent chain** guarantees the first step matches the exact shortest path produced by BFS, removing the “go LEFT when there’s no reason” symptom you observed. The broken old loops look like `while (dist[gy][gx] > 1) { ... pick neighbor with dist-1 ... }` — replace all of that logic with `reconstruct_first_step_dir(...)`. See next section for where to use it. The old fragments appear repeatedly in your file (duplicate code blocks), so update them all.&#x20;

---

# 2) Wherever you choose a target, take the **first step** via `reconstruct_first_step_dir`

Find each block that currently does manual “walk-back by dist” to get a direction and **replace it** with:

```cpp
int dir = reconstruct_first_step_dir(G, sy, sx, target_y, target_x);
if (dir == -1) {
    // unreachable or already here -> fall back
    // e.g., survival_strategy or next candidate
} else {
    // prevent 180° reverse
    int opposite_dir = (s.self().dir + 2) % 4;
    if (dir == opposite_dir) {
        // fall back to safety if reversing would be illegal
        int choice = survival_strategy(s, sy, sx, log_ss, M);
        str_info += log_ss.str();
        return {choice};
    }
    // use dir
    log_ss << "MULTI_TARGET_MOVE:" << dir << "|";
    str_info += log_ss.str();
    return {ACT[dir]};
}
```

> You have multiple near-identical “finalize move” sites (also with reverse-direction checks). Update them all to use `reconstruct_first_step_dir` rather than trying to deduce the step from raw `dist`. The reverse-direction safe-guard you already have is fine; just keep it after you compute `dir`.&#x20;

---

# 3) Correct chest scoring inside `items` (use **real** chest score)

Right now, you assign chests a fixed internal value when parsing `items`:

```cpp
case -5: // 宝箱
    s.items[i].value = CHEST_VALUE;  // <-- fixed 100
    break;
```

Then you read **real** chest scores later into `s.chests[...]`. Make the values consistent by **updating the `items` entries after reading chests**:

```cpp
// After reading chests (keep your existing code to fill s.chests)
for (const auto& chest : s.chests) {
    for (auto& it : s.items) {
        if (it.type == -5 && it.pos.y == chest.pos.y && it.pos.x == chest.pos.x) {
            it.value = chest.score * CHEST_SCORE_MULTIPLIER;         // <-- use actual score
            break;
        }
    }
}
```

Place that right after `for (int i = 0; i < nc; i++) cin >> s.chests[i].pos.y ...;`

> Without this, your scoring keeps treating chests as a flat constant, which misprioritizes routes.&#x20;

---
