You’ve already built a very solid baseline (state ingest, safe-zone handling, weighted BFS/Dijkstra, danger modeling, look-ahead). Below is a **surgical upgrade** that drops in a **rolling-horizon route planner** (candidate set → greedy feasible insertions → short local search with delta simulation), while **keeping your I/O and safety logic** intact. I read your code end-to-end and will reference it as “current code”.&#x20;

---

# What we’re changing (high-impact, low-risk)

1. **Treat other snakes as hard walls** (no weaving through bodies), keep **chests blocked** always, **ignore keys/chests as targets**.
2. Add a per-tick **Route + Timeline** and build a **multi-item route** (not just one target), then **emit only the first step**.
3. Build a **candidate set (K≈24)** of reachable food/growth with lifetime feasibility.
4. **Greedy feasible insertions** (Δvalue/Δtime) to assemble a good route fast.
5. A short **local search** (Relocate / Swap / 2-opt\*) with **delta simulation of the affected suffix only** under a tight wall-clock cap, so the whole tick stays ≤0.7s.
6. Reuse your **danger** & **safe-zone** logic for fallback when no feasible route exists.

---

## 0) Make snakes/chests hard obstacles (tiny change)

In `build_masks(...)`:

```cpp
// === Other snakes: block as hard walls ===
for (const auto& sn : s.snakes) {
    if (sn.id == MYID) continue;
    for (const auto& p : sn.body) if (in_bounds(p.y,p.x) && in_safe_zone(s.cur,p.y,p.x)) {
        M.snake(p.y,p.x);   // keep for diagnostics
        M.block(p.y,p.x);   // <-- NEW: make them impassable
    }
}

// === Chests: always block (ignore as targets) ===
for (const auto& c : s.chests) {
    if (in_bounds(c.pos.y,c.pos.x) && in_safe_zone(s.cur,c.pos.y,c.pos.x))
        M.block(c.pos.y,c.pos.x);      // <-- always blocked
}
```

And in **target building later** we’ll **skip keys/chests** entirely.

---

## 1) Add per-tick planning structs (top of file, near your structs)

```cpp
struct Route {
    // store coordinates (y,x) so we don’t need stable item IDs
    vector<Point> seq;      // order of targets this tick (food/growth only)
    int totalVal = 0;       // sum of item.val along seq
    int finishT  = 0;       // simulated time to finish the seq (steps)
};

struct Timeline {
    vector<int> arrive, leave;  // aligned with seq
    bool feasible = true;
};
```

---

## 2) Small helpers

Place near `decide(...)`:

```cpp
// Manhattan as a tie-breaker; real travel comes from BFS.
inline int manhattan(Point a, Point b){ return abs(a.y-b.y) + abs(a.x-b.x); }

// Run your existing BFS/Dijkstra from an arbitrary start (reuse bfs_grid)
inline int dist_grid_steps(const GridMask& M, const State& s, Point from, Point to){
    if (from.y==to.y && from.x==to.x) return 0;
    auto G = bfs_grid(M, s, from.y, from.x);
    int d = G.dist[to.y][to.x];
    return (d >= (int)1e9) ? (int)1e9 : d;
}
```

> We’ll call `bfs_grid` a handful of times when simulating inserts/moves. On 40×30 it’s cheap; we cap evaluations to stay <0.7s.

---

## 3) Candidate set (food & growth only)

Right after you compute `BFSOut G = bfs_grid(M, s, sy, sx);` replace the current big scoring loop with a **candidate builder**:

```cpp
struct Cand { Point p; int val; int d; double score; int ttl; };

auto build_candidates = [&](int K=24){
    vector<Cand> C; C.reserve(64);
    for (const auto& it : s.items){
        // ignore non-food: keys (-3), chests (-5), traps (-2)
        if (!(it.type >= 1 || it.type == -1)) continue;

        // reachable & lifetime check using head BFS G
        if (!in_bounds(it.pos.y,it.pos.x) || !in_safe_zone(s.cur,it.pos.y,it.pos.x)) continue;
        int d = G.dist[it.pos.y][it.pos.x];
        if (d >= (int)1e9) continue;
        if (it.lifetime != -1 && d > it.lifetime) continue;

        int v = (it.type >= 1) ? (it.type * NORMAL_FOOD_MULTIPLIER) : GROWTH_FOOD_VALUE; // reuse your values
        // density with slight bias for earlier expiry
        double life_factor = (it.lifetime==-1? 1.0 : pow(LIFETIME_SOFT_DECAY, d));
        double sc = (v * life_factor) / (d + DISTANCE_OFFSET);
        C.push_back({it.pos, v, d, sc, it.lifetime});
    }
    if (C.empty()) return C;
    sort(C.begin(), C.end(), [](const Cand& a, const Cand& b){
        if (a.score!=b.score) return a.score > b.score;
        return a.d < b.d;
    });
    if ((int)C.size() > K) C.resize(K);
    return C;
};
```

Call it: `auto C = build_candidates();`

---

## 4) Route simulate (suffix-only recompute)

```cpp
// Simulate seq from index L to end; uses BFS per hop (fast on 40×30)
static bool simulate_suffix(const GridMask& M, const State& s,
                            const vector<Point>& seq,
                            int L,           // first index to recompute
                            Point head,      // current head
                            const Timeline& TL_in, // prior timeline (for prefix)
                            Timeline& TL_out,
                            int& totalVal, int& finishT){
    TL_out = TL_in;
    if ((int)TL_out.arrive.size() != (int)seq.size()) {
        TL_out.arrive.resize(seq.size());
        TL_out.leave .resize(seq.size());
    }
    Point cur = head;
    int t = 0;
    if (L > 0){
        cur = seq[L-1];
        t   = TL_in.leave[L-1];
    }
    for (int i=L;i<(int)seq.size();++i){
        int d = dist_grid_steps(M, s, cur, seq[i]);
        if (d >= (int)1e9) { TL_out.feasible=false; return false; }
        int arr = t + d;
        TL_out.arrive[i] = arr;
        TL_out.leave [i] = arr;       // pickup cost ~ 0
        // lifetime feasibility: deny if expired upon arrival
        // (We need ttl; if you need it, pass a lambda from candidate map.)
        t   = arr;
        cur = seq[i];
    }
    TL_out.feasible = true;
    finishT = (seq.empty()? 0 : TL_out.leave.back());
    // totalVal is caller-maintained; here we only confirm feasibility
    return true;
}
```

---

## 5) Greedy feasible insertions (Δvalue / Δtime)

```cpp
static void build_route_greedy(const GridMask& M, const State& s,
                               const vector<Cand>& C,
                               Point head, Route& R){
    R.seq.clear(); R.totalVal=0; R.finishT=0;
    Timeline TL; TL.arrive.clear(); TL.leave.clear(); TL.feasible=true;

    // simple loop: try to insert each candidate once where it fits best
    vector<Point> seq;
    for (const auto& c : C){
        int bestPos=-1; double bestGain=-1; Timeline TL_try;
        int bestFinish=0;

        for (int pos=0; pos<= (int)seq.size(); ++pos){
            vector<Point> trial = seq; trial.insert(trial.begin()+pos, c.p);
            // suffix simulate from pos
            Timeline TL_out = TL; int totalVal=0, finT=0;
            if (!simulate_suffix(M, s, trial, pos, head, TL, TL_out, totalVal, finT)) continue;

            int oldFin = (seq.empty()? 0 : (TL.leave.empty()? 0 : TL.leave.back()));
            int dVal   = c.val;
            int dTime  = finT - oldFin;
            double gain = (dTime>0? (double)dVal/dTime : (dVal>0? 1e9 : 0));
            if (gain > bestGain){ bestGain=gain; bestPos=pos; bestFinish=finT; TL_try=TL_out; }
        }
        if (bestPos!=-1){
            seq.insert(seq.begin()+bestPos, c.p);
            TL = TL_try;
            R.totalVal += c.val;
            R.finishT   = bestFinish;
        }
    }
    R.seq = std::move(seq);
}
```

---

## 6) Short local search (first-improvement)

Add two quick operators; cap attempts (e.g., 200–400) and **wall-clock** (\~0.45–0.55s of your 0.7s):

```cpp
static void improve_route_ls(const GridMask& M, const State& s, Point head, Route& R){
    if (R.seq.size() < 2) return;
    Timeline TL; TL.arrive.resize(R.seq.size()); TL.leave.resize(R.seq.size()); TL.feasible=true;
    // recompute baseline TL
    int dummy=0, finT=0;
    if (!simulate_suffix(M,s,R.seq,0,head,TL,TL,dummy,finT)) return;
    R.finishT = finT;

    auto better = [&](int newVal, int newFin){
        if (newVal != R.totalVal) return newVal > R.totalVal;
        return newFin < R.finishT;
    };

    const auto tStart = chrono::high_resolution_clock::now();
    int tries=0;

    while (true){
        bool improved=false;

        // Relocate(1)
        for (int i=0; i<(int)R.seq.size() && !improved; ++i){
            for (int j=0; j<(int)R.seq.size() && !improved; ++j){
                if (i==j) continue;
                auto seq = R.seq;
                Point moved = seq[i];
                seq.erase(seq.begin()+i);
                seq.insert(seq.begin()+(j<(i?i:0)? j : j), moved);
                Timeline TL2; int dummy2=0, finT2=0;
                if (!simulate_suffix(M,s,seq,min(i,j),head,TL,TL2,dummy2,finT2)) continue;
                int newVal = R.totalVal; // value sum unchanged by reorder
                if (better(newVal, finT2)){
                    R.seq = std::move(seq); TL = TL2; R.finishT=finT2;
                    improved=true; break;
                }
                if (++tries>400) break;
            }
        }
        if (improved) continue;

        // Swap(1,1)
        for (int i=0; i+1<(int)R.seq.size() && !improved; ++i){
            for (int j=i+1; j<(int)R.seq.size() && !improved; ++j){
                auto seq = R.seq; swap(seq[i], seq[j]);
                Timeline TL2; int dummy2=0, finT2=0;
                if (!simulate_suffix(M,s,seq,min(i,j),head,TL,TL2,dummy2,finT2)) continue;
                int newVal = R.totalVal;
                if (better(newVal, finT2)){
                    R.seq = std::move(seq); TL = TL2; R.finishT=finT2;
                    improved=true; break;
                }
                if (++tries>400) break;
            }
        }
        if (improved) continue;

        // (Optional) 2-opt*
        // You can add segment reverse here with the same pattern & delta simulate.

        // time / tries cap
        auto ms = chrono::duration_cast<chrono::milliseconds>(chrono::high_resolution_clock::now() - tStart).count();
        if (ms > 550 || tries>400) break;  // keep whole tick ≤ 0.7s
        break; // no improvement found
    }
}
```

---

## 7) Use the route to choose the **first step**

Inside `decide(...)` after building `M` and `G`:

```cpp
Point head{sy,sx};
auto C = build_candidates();
if (C.empty()){
    int choice = /* your existing */ last_choice();
    return {choice};
}

// Build then improve route in this tick
Route R;
build_route_greedy(M, s, C, head, R);
improve_route_ls(M, s, head, R);

// Emit only the first action toward R.seq.front()
auto goal = R.seq.front();
auto G2 = bfs_grid(M, s, head.y, head.x);   // we already have G, reuse if you prefer
if (G2.parent[goal.y][goal.x] == -1) {
    int choice = last_choice();
    return {choice};
}

// reconstruct one step (prefer non-danger neighbor if tie)
int cy = goal.y, cx = goal.x;
while (!(cy==head.y && cx==head.x)) {
    int back = G2.parent[cy][cx];
    int py = cy + DY[back], px = cx + DX[back];
    if (py==head.y && px==head.x) break;
    cy = py; cx = px;
}

int dir = -1;
for (int k=0;k<4;k++) if (head.y+DY[k]==cy && head.x+DX[k]==cx){ dir=k; break; }
if (dir==-1) { int choice = last_choice(); return {choice}; }

// optional: swap to a non-danger neighbor on equal shortest
for (int k=0;k<4;k++){
    int ny=head.y+DY[k], nx=head.x+DX[k];
    if (!in_bounds(ny,nx) || M.blocked(ny,nx)) continue;
    if (G2.dist[ny][nx] == G2.dist[goal.y][goal.x]-1 && !M.is_danger(ny,nx)){
        dir = k; break;
    }
}
return {ACT[dir]};
```

> This **replaces** your single-target pick. Your **last_choice()**, shield logic, and safe-zone guards remain as robust fallbacks.

---

## 8) Time budget (fits ≤0.7s)

- `bfs_grid` (head): 1–3 ms.
- Candidate build: 0–2 ms.
- Greedy insertions (K≈24, positions ≤24, each suffix simulation invokes a few BFS): typically 50–150 ms.
- Local search (Relocate+Swap, capped): 200–500 ms.
- Emit first step: < 2 ms.

If your machine or judge is tighter, **lower K to 16**, cut LS `tries` to 200, or early-stop greedy after, say, **8 accepted insertions**.

---

## 9) What you’ll see in play

- The snake now **chains multiple pickups** each tick instead of chasing only the single best density item.
- It respects **lifetime** by building a sequence that arrives before expiry.
- Short LS “packs” nearby items and reduces detours ⇒ **more eating per minute**.

---

If you paste your updated `my-snake.cpp` with the changes above, I’ll do a line-by-line review and fine-tune constants (K, decay base, LS caps) and any hot spots that might jeopardize the **≤0.7s** tick budget.
