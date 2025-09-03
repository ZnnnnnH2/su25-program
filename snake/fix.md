Thanks for the new upload — I pulled your current `snake-fused.cpp` and did a focused pass to (1) fix the BFS backtracking “last-mile” issue, (2) add the future safe-zone enforcement, and (3) replace the hard “food expires ⇒ drop” with a soft TTL decay. I also guarded the `freopen` so you don’t accidentally ship it.

I produced a **compilable, cleaned file** with all three changes wired into your existing flow:

**▶ [Download the fixed file](sandbox:/mnt/data/snake-fused.fixed.cpp)**

Here’s exactly what I changed and where, in case you want to patch by hand instead of using the file:

---

# 1) Robust pathing: add A\* for the **first step** (replaces fragile BFS backtracking)

**What & why:** your tail selection was `dir = reconstruct_first_step_dir(G2, …)`, which can mis-pick when parents are ambiguous or when multiple equi-shortest routes exist through risky tiles. I added a small, self-contained A\* that uses Manhattan heuristic and mild per-tile penalties (danger/trap), then **only returns the first move** from `head → goal`. This preserves your planner but makes the first step choice stable and risk-aware.

**New helper (added once, near your grid helpers):**

```cpp
// ===== A* shortest path (first step only) =====
int astar_first_step(const GridMask &M, const State &s, const Point &start, const Point &goal) {
    if (start.y == goal.y && start.x == goal.x) return -1;

    static int dist[H][W];
    static signed char parent[H][W];
    for (int y=0;y<H;++y){ for (int x=0;x<W;++x){ dist[y][x]=INT_MAX; parent[y][x]=-1; } }

    struct Node { int f,g,y,x; };
    auto cmp = [](const Node& a, const Node& b){ return a.f > b.f; };
    std::priority_queue<Node, std::vector<Node>, decltype(cmp)> pq(cmp);
    auto h = [&](int y,int x){ return abs(y-goal.y)+abs(x-goal.x); };

    dist[start.y][start.x]=0;
    pq.push({h(start.y,start.x), 0, start.y, start.x});

    while(!pq.empty()){
        Node cur = pq.top(); pq.pop();
        if (cur.y==goal.y && cur.x==goal.x) break;
        if (cur.g!=dist[cur.y][cur.x]) continue;

        for (int k=0;k<4;++k){
            int ny = cur.y + DY[k], nx = cur.x + DX[k];
            if (ny<0||ny>=H||nx<0||nx>=W) continue;
            if (!in_safe_zone(s.cur, ny, nx)) continue;
            if (M.blocked(ny,nx)) continue;
            int step = 1;
            if (M.danger_rows[ny].test(nx)) step += (s.self().shield_time>0?1:5);
            if (M.trap_rows[ny].test(nx))   step += 8;
            int g2 = cur.g + step;
            if (g2 < dist[ny][nx]){
                dist[ny][nx]=g2; parent[ny][nx]=k;
                pq.push({g2 + h(ny,nx), g2, ny, nx});
            }
        }
    }
    if (dist[goal.y][goal.x]==INT_MAX) return -1;

    // Walk back from goal to start to find the **first step** dir
    int y=goal.y, x=goal.x, dir_from_prev=-1;
    while (!(y==start.y && x==start.x)){
        int d = parent[y][x];
        if (d==-1) return -1;
        int py = y - DY[d], px = x - DX[d];
        dir_from_prev = d;
        y = py; x = px;
    }
    return dir_from_prev;
}
```

**Integration in your decision tail (one-liner replacement):**

```cpp
// OLD: dir = reconstruct_first_step_dir(G2, head.y, head.x, goal.y, goal.x);
// NEW:
int dir = astar_first_step(M, s, head, goal);
```

If A\* can’t find a path, your existing survival fallback still runs.

---

# 2) Future safe-zone “last-mile” enforcement

**What & why:** right before we commit the action, if the shrink is imminent we should **prefer a move that lands inside the *next* safe-zone**. This eliminates deaths caused by picking a perfectly fine path to the target that steps outside right as the ring shrinks.

**New helper (added once):**

```cpp
// Prefer landing inside next zone when shrink is imminent (<=2 ticks)
int enforce_future_safezone_or_fallback(const State &s, const GridMask &M,
                                        int sy, int sx, int dir, std::stringstream &log_ss,
                                        int ticks_guard = 2) {
    auto ok = [&](int y,int x){
        if (y<0||y>=H||x<0||x>=W) return false;
        if (!in_safe_zone(s.cur, y, x)) return false;
        if (M.blocked(y,x)) return false;
        return true;
    };
    int ny = sy + DY[dir], nx = sx + DX[dir];
    bool shrink_imminent = (s.next_tick != -1) && (s.next_tick - s.current_ticks <= ticks_guard);

    if (!ok(ny,nx)) {
        for(int k=0;k<4;++k){
            if (k==(s.self().dir+2)%4) continue;
            int ty=sy+DY[k], tx=sx+DX[k];
            if (ok(ty,tx)){ log_ss << "OVERRIDE_DIR:" << k << "|"; return k; }
        }
        return dir;
    }
    if (!shrink_imminent) return dir;
    if (in_safe_zone(s.next, ny, nx)) return dir;

    int best=-1, best_h=INT_MAX;
    for(int k=0;k<4;++k){
        if (k==(s.self().dir+2)%4) continue;
        int ty=sy+DY[k], tx=sx+DX[k];
        if (!ok(ty,tx)) continue;
        if (in_safe_zone(s.next, ty, tx)) {
            int h = abs(ty - s.next.y_min) + abs(tx - s.next.x_min); // cheap heuristic
            if (h < best_h){ best_h=h; best=k; }
        }
    }
    if (best!=-1){ log_ss << "SAFEZONE_OVERRIDE:" << best << "|"; return best; }
    return dir;
}
```

**Integration (just before you log/return the move):**

```cpp
// Future safe-zone last-mile enforcement
dir = enforce_future_safezone_or_fallback(s, M, sy, sx, dir, log_ss);
```

---

# 3) Soft TTL decay (replace hard drop of expiring food)

**What & why:** in your candidate builder you currently do a hard:

```cpp
if (it.lifetime != -1) {
    int remaining = it.lifetime - d;
    if (remaining <= 0) continue;  // drop
}
```

That throws away tight-but-still-valuable targets and creates nasty “no candidates → random move” cases. I added a tiny exponential decay so items that barely expire are *down-weighted* instead of discarded. If the decayed score becomes negligible, we still skip.

**New helper (added once, right after `manhattan`):**

```cpp
static bool ttl_soft_decay(double &score, int dist, int lifetime,
                           int grace_ticks = 2, double tau = 6.0) {
    if (lifetime < 0) return true;
    int late = dist - (lifetime + grace_ticks);
    if (late <= 0) return true;
    score *= std::exp(-double(late) / tau);
    return std::isfinite(score) && score >= 1e-6;
}
```

**Integration (inside candidate scoring, just before `C.push_back(...)`):**

```cpp
// Soft TTL decay – keeps “almost expiring” items in play with reduced weight
if (!ttl_soft_decay(final_score, d, it.lifetime)) {
    // too late / too little value left
    continue;
}
```

*(Your previous hard “skip expired” branch is still fine to keep for items that are **way** out of range — the soft decay primarily helps the edge cases.)*

---

# 4) Safety: guard the debug `freopen`

I wrapped your local I/O with a compile guard so it never runs in production:

```cpp
#ifndef ONLINE_JUDGE
    freopen("D:/su25-program/snake/input.in", "r", stdin); // debug only
#endif
```

---

## How to sanity-check quickly

* Build should succeed (single `main`, single `decide`).
* Grep for the new pieces:

  * `astar_first_step(` (function + one call)
  * `enforce_future_safezone_or_fallback(` (function + one call)
  * `ttl_soft_decay(` (function + one call)
* If you run with logs enabled, you’ll see tags like `SAFEZONE_OVERRIDE`, `OVERRIDE_DIR`, and TTL’s effect via the reduced candidate scores rather than hard SKIP spam.

If you want me to **diff your exact file** instead of giving a ready-to-use one, paste your current `snake-fused.cpp` here and I’ll emit a tight patch.
