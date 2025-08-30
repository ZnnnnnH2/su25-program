Great—let’s harden the “no-self-collision,” re-define enemy-body collision, **add dodge-first behavior**, and **stop burning shield for tiny gains**. Below are drop-in patches and where to put them. They’re minimal and focused on what you asked:

---

# 1) Map modeling: enemy body vs. danger halo (dodge-first)

### ✅ What changes

* Keep **your own body passable** (you already skip it—good).
* Treat **enemy bodies as hard walls** (blocked), but also create a **soft “danger halo”** around them to prefer paths with clearance.
* Also mark **enemy heads themselves** as danger (you already mark neighbors).

### Patch A — extend `GridMask`

Add an adjacency halo bitset so BFS can add a *soft* cost near enemy bodies (not a block).

```cpp
struct GridMask
{
    bitset<W> blocked_rows[H]; // hard walls
    bitset<W> snake_rows[H];   // enemy bodies
    bitset<W> danger_rows[H];  // enemy head next-cells (+ heads)
    bitset<W> near_rows[H];    // NEW: cells adjacent to enemy bodies (soft danger)

    inline void block(int y, int x){ if(in_bounds(y,x)) blocked_rows[y].set(x); }
    inline void snake(int y, int x){ if(in_bounds(y,x)) snake_rows[y].set(x); }
    inline void danger(int y, int x){ if(in_bounds(y,x)) danger_rows[y].set(x); }
    inline void near(int y, int x){ if(in_bounds(y,x)) near_rows[y].set(x); }

    inline bool blocked(int y,int x)const{ return in_bounds(y,x)?blocked_rows[y].test(x):true; }
    inline bool is_snake(int y,int x)const{ return in_bounds(y,x)?snake_rows[y].test(x):false; }
    inline bool is_danger(int y,int x)const{ return in_bounds(y,x)?danger_rows[y].test(x):true; }
    inline bool is_near(int y,int x)const{ return in_bounds(y,x)?near_rows[y].test(x):false; }
};
```

### Patch B — build the halo in `build_masks`

Add: (1) mark enemy heads themselves as danger; (2) mark cells *adjacent to enemy bodies* as “near”.

```cpp
// 2) mark ENEMY bodies as blocked (you already do)
for (const auto &sn : s.snakes) {
    if (sn.id == MYID) continue;
    for (const auto &p : sn.body) if (in_bounds(p.y,p.x)) {
        M.snake(p.y,p.x);      // remember enemy body
        M.block(p.y,p.x);      // hard wall
    }
}

// 3) chests & traps already blocked (good)

// 5) enemy-head danger + neighbors (you had neighbors; add head cell itself)
for (const auto &sn : s.snakes) {
    if (sn.id == MYID) continue;
    auto h = sn.head();
    if (in_bounds(h.y,h.x) && in_safe(s.cur,h.y,h.x)) M.danger(h.y,h.x); // head itself
    for (int k=0;k<4;k++){
        int ny=h.y+DY[k], nx=h.x+DX[k];
        if (in_bounds(ny,nx) && in_safe(s.cur,ny,nx)) M.danger(ny,nx);
    }
}

// 6) NEW: body-adjacent halo (soft)
for (int y=0;y<H;y++) for (int x=0;x<W;x++) if (M.is_snake(y,x)) {
    for (int k=0;k<4;k++){
        int ny=y+DY[k], nx=x+DX[k];
        if (in_bounds(ny,nx) && !M.blocked(ny,nx)) M.near(ny,nx);
    }
}
```

---

# 2) Pathfinding cost: avoid enemy body (and *near* it) without auto-shield

### ✅ What changes

* Make **enemy-body cells extremely expensive** (but not strictly forbidden in the PQ to allow “survival last resort”).
* Add a **small cost** for the halo (`near`) to prefer clearance.
* This makes the snake naturally “dodge around” enemies instead of thinking “I’ll shield through”.

### Patch C — tweak `bfs_grid`

Raise body cost, add halo cost, and simplify the priority condition.

```cpp
static BFSOut bfs_grid(const GridMask &M, const State &s, int sy, int sx)
{
    BFSOut out;
    for (int y=0;y<H;y++) for (int x=0;x<W;x++) {
        out.dist[y][x]=1e9; out.snake_cost[y][x]=0; out.parent[y][x]=-1;
    }

    // weights (tunable):
    const int W_BODY  = 200;  // cost per enemy-body cell traversed (very high)
    const int W_NEAR  = 3;    // small bias to keep clearance
    const int W_STEP  = 1;    // step cost

    using Node=tuple<int,int,int,int>; // (F, snake_cost, y, x)
    priority_queue<Node, vector<Node>, greater<>> pq;

    out.dist[sy][sx]=0;
    pq.emplace(0, 0, sy, sx);

    auto f_of = [&](int d, int sc){ return d*W_STEP + sc*W_BODY; };

    while(!pq.empty()){
        auto [F, sc, y, x] = pq.top(); pq.pop();
        // current best?
        if (F != f_of(out.dist[y][x], out.snake_cost[y][x])) continue;

        for(int k=0;k<4;k++){
            int ny=y+DY[k], nx=x+DX[k];
            if (!in_bounds(ny,nx) || M.blocked(ny,nx)) continue;

            int nd = out.dist[y][x] + 1;
            int nsc = out.snake_cost[y][x];

            // strong penalty for stepping on enemy body
            if (M.is_snake(ny,nx)) nsc += 1;

            // soft penalty near enemy body
            int nF = f_of(nd, nsc) + (M.is_near(ny,nx) ? W_NEAR : 0);

            int curF = f_of(out.dist[ny][nx], out.snake_cost[ny][nx]);
            if (nF < curF){
                out.dist[ny][nx]=nd;
                out.snake_cost[ny][nx]=nsc;
                out.parent[ny][nx]=(k+2)%4;
                pq.emplace(nF, nsc, ny, nx);
            }
        }
    }
    return out;
}
```

---

# 3) Stop “small gain” shield usage + add dodge-first fallback

### ✅ What changes

* **NEVER** pop shield for tiny rewards. Set a clear threshold and only allow if:

  1. We’re **dead without it** (no safe moves); or
  2. We’re securing **big value** (e.g., chest/key/large food) above a threshold.
* When the chosen first step lands on **enemy body** or **danger**, try **side-dodges** before considering shield.

### Patch D — add a helper to evaluate whether it’s worth shielding

Put this near the top (after structs), or make it `static` inside the file:

```cpp
struct Worth {
    bool ok;        // allow shield?
    const char* why;
};

static Worth should_use_shield(const State& s, int target_value, bool survival_only=false){
    const auto &me = s.self();

    // forbid using shield when gain is small
    const int MIN_GAIN_TO_SHIELD = 15;   // tunable; “too small” -> no shield
    if (!survival_only && target_value < MIN_GAIN_TO_SHIELD) return {false, "small"};

    // never if shield not ready
    if (me.shield_time>0) return {true, "already_on"};
    if (me.shield_cd>0)   return {false, "cooling"};

    // additional late-game leniency (optional)
    if (!survival_only && s.remaining_ticks < 30) return {true, "late"};

    return {true, "ok"};
}
```

### Patch E — compute target “value” for shield decisions

Right after you’ve picked `target` in `decide` (you already compute `v` when building candidates), keep the **numeric value** for shield decisions:

* Add `int target_value;` to `Target`
* When pushing candidates, set `target_value`:

```cpp
struct Target{
    int y,x;
    double score;
    int dist;
    int snake_cost;
    bool grows;
    int value; // NEW: raw value for shield policy
};

// When pushing items
cand.push_back({it.pos.y, it.pos.x, sc, d, snake_steps, grows, (int)round(v)});

// When pushing chests
cand.push_back({c.pos.y, c.pos.x, sc, d, snake_steps, false, (c.score>0?c.score:60)});
```

### Patch F — dodge-first if first step is risky, only then consider shield

Replace your sections (8) and (9) in `decide` with this block right **after** you compute `dir` and before returning:

```cpp
// --- Risk checks for the chosen first step (cy,cx) -> dir ---

auto try_dodge = [&]()->int{
    // choose the safest neighbor by: not blocked, not enemy body,
    // lowest danger, lowest near-penalty, many degrees
    int best=-1, bestScore=-1;
    for (int k=0;k<4;k++){
        int ny=sy+DY[k], nx=sx+DX[k];
        if (!in_bounds(ny,nx) || M.blocked(ny,nx) || M.is_snake(ny,nx)) continue;
        // prefer non-danger, non-near, higher free-degree
        int deg=0; for (int t=0;t<4;t++){
            int py=ny+DY[t], px=nx+DX[t];
            if (in_bounds(py,px) && !M.blocked(py,px) && !M.is_snake(py,px)) deg++;
        }
        int score = (M.is_danger(ny,nx)?0:2) + (M.is_near(ny,nx)?0:1) + deg;
        if (score>bestScore){ bestScore=score; best=k; }
    }
    return best; // -1 if none
};

// 8) hard risk: enemy-head zone or enemy body
bool step_danger = M.is_danger(cy,cx);
bool step_body   = M.is_snake(cy,cx);

if (step_body || step_danger){
    // Try to dodge first
    int alt = try_dodge();
    if (alt != -1){
        str_info += "|DJ" + to_string(ACT[alt]); // log: Dodged
        return {ACT[alt]};
    }

    // If no dodge, consider shield policy:
    // - If stepping into BODY: only allow if survival or big gain.
    // - If stepping into DANGER (head fight): same rule.
    auto gate = should_use_shield(s, target.value, /*survival_only=*/false);
    if (!gate.ok){
        // As we are forced, attempt any legal non-body move (even if danger)
        for (int k=0;k<4;k++){
            int ny=sy+DY[k], nx=sx+DX[k];
            if (in_bounds(ny,nx) && !M.blocked(ny,nx) && !M.is_snake(ny,nx))
                return {ACT[k]};
        }
        // survival-only: if literally no other output, allow shield if ready
        gate = should_use_shield(s, /*tiny*/0, /*survival_only=*/true);
        if (gate.ok) return {4};
        // otherwise just pick dir (may die, but policy respected)
        return {ACT[dir]};
    } else {
        // Shield allowed (big gain or late game) — but only if shield is actually available
        const auto &me = s.self();
        if (me.shield_time>0 || me.shield_cd==0) return {4};
        // else: do our best without shield
        int alt2 = try_dodge();
        if (alt2!=-1) return {ACT[alt2]};
        return {ACT[dir]};
    }
}

// 9) normal safe case
return {ACT[dir]};
```

This block:

* **DODGES first** when the chosen step is into enemy body or head-danger.
* Only **permits shield** if the **target value is big enough** (configurable) or **survival-only**.
* If shield is on cooldown, it **still won’t** force shield; it seeks any non-body step.

---

# 4) Tiny clean-ups

* You set fast-IO in both `main()` and `read_state()`. Keep it **only in `main()`** to avoid weirdness under some judges.
  Remove these lines from `read_state`:

  ```cpp
  ios::sync_with_stdio(false);
  cin.tie(nullptr);
  ```

---

## TL;DR behavior changes you’ll see

* Your snake **won’t rely on shield** for +1/+2 crumbs; it will **dodge enemy bodies** and their halo first.
* It **won’t collide with your own body** (you already modeled that—kept intact).
* It uses a **global BFS** but now assigns **heavy cost to enemy bodies** and a **soft cost near them**, which naturally pushes it to **skirt around** opponents.
* Shield now fires **only for survival** or **high-value grabs** (key/chest/large score), not just “because a cell is scary”.

If you paste these patches into your file (A–F), you’ll get the new behavior without changing your public API or the overall structure. Want me to integrate them into your `/mnt/data/snake2.cpp` in one unified diff?
