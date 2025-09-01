#include <bits/stdc++.h>
using namespace std;

// ==================== Game Constants ====================
static constexpr int W = 40;            // Map width
static constexpr int H = 30;            // Map height
static constexpr int MYID = 2024201540; // Student ID

// ==================== Shield and Movement Cost Constants ====================
static constexpr int SNAKE_COST_WEIGHT = 10;     // Snake body cost weight
static constexpr int SNAKE_COST_NO_SHIELD = 100; // High cost without shield
static constexpr int SNAKE_COST_WITH_SHIELD = 0; // Cost with shield active
static constexpr int SNAKE_COST_OPEN_SHIELD = 2; // Cost to activate shield
static constexpr int SHIELD_COST_THRESHOLD = 20; // Min score to use shield
static constexpr int TRAP_STEP_COST = 30;        // Trap penalty in pathfinding
static constexpr int NEAR_SNAKE_ADJ_PENALTY = 5; // Penalty for adjacent to snake body

// ==================== Item Value Constants ====================
static constexpr int GROWTH_FOOD_VALUE = 8;      // Growth food value
static constexpr int TRAP_PENALTY = -10;         // Trap deduction
static constexpr int KEY_VALUE = 50;             // Key value
static constexpr int CHEST_BASE_VALUE = 30;      // Base chest value
static constexpr int NORMAL_FOOD_MULTIPLIER = 4; // Normal food multiplier

// ==================== Scoring Weight Constants ====================
static constexpr double SNAKE_SAFETY_PENALTY_RATE = 0.5; // Snake crossing penalty rate
static constexpr double CHEST_SCORE_MULTIPLIER = 1.5;    // Chest score multiplier
static constexpr double DISTANCE_OFFSET = 1.0;           // Distance offset to avoid div by zero
static constexpr double LIFETIME_SOFT_DECAY = 0.9;       // Lifetime decay factor
static constexpr double CONTEST_PENALTY = 0.5;           // Competition penalty
static constexpr double NEXTZONE_RISK_PENALTY = 0.35;    // Next zone risk penalty
static constexpr double DEGREE_BONUS = 0.06;             // Local freedom bonus

// ==================== Data Structures ====================
struct Point
{
    int y, x;
};

struct Item
{
    Point pos;
    int type;
    int value;
    int lifetime;
};

struct Snake
{
    int id, length, score, dir;
    int shield_cd, shield_time;
    bool has_key = false;
    vector<Point> body;
    const Point &head() const { return body.front(); }
};

struct Chest
{
    Point pos;
    int score;
};

struct Key
{
    Point pos;
    int holder_id;
    int remaining_time;
};

struct Safe
{
    int x_min, y_min, x_max, y_max;
};

struct State
{
    int current_ticks;
    int remaining_ticks;
    vector<Item> items;
    vector<Snake> snakes;
    vector<Chest> chests;
    vector<Key> keys;
    Safe cur, next, fin;
    int next_tick = -1, fin_tick = -1;
    int self_idx = -1;

    const Snake &self() const { return snakes[self_idx]; }
};

// Global state
State global_state;
string str_info;

// Movement directions: 0=left, 1=up, 2=right, 3=down
static const int DX[4] = {-1, 0, 1, 0};
static const int DY[4] = {0, -1, 0, 1};
static const int ACT[4] = {0, 1, 2, 3};

// ==================== Input Reading ====================
static void read_state(State &s)
{
    if (!(cin >> s.remaining_ticks))
        exit(0);
    s.current_ticks = 256 - s.remaining_ticks;

    // Read items
    int m;
    cin >> m;
    s.items.resize(m);
    for (int i = 0; i < m; i++)
    {
        cin >> s.items[i].pos.y >> s.items[i].pos.x >> s.items[i].type >> s.items[i].lifetime;
        switch (s.items[i].type)
        {
        case -1:
            s.items[i].value = GROWTH_FOOD_VALUE;
            break;
        case -2:
            s.items[i].value = TRAP_PENALTY;
            break;
        case -3:
            s.items[i].value = KEY_VALUE;
            break;
        case -5:
            s.items[i].value = CHEST_BASE_VALUE;
            break;
        default:
            s.items[i].value = s.items[i].type * NORMAL_FOOD_MULTIPLIER;
            break;
        }
    }

    // Read snakes
    int ns;
    cin >> ns;
    s.snakes.resize(ns);
    unordered_map<int, int> id2idx;
    for (int i = 0; i < ns; i++)
    {
        auto &sn = s.snakes[i];
        cin >> sn.id >> sn.length >> sn.score >> sn.dir >> sn.shield_cd >> sn.shield_time;
        sn.body.resize(sn.length);
        for (int j = 0; j < sn.length; j++)
            cin >> sn.body[j].y >> sn.body[j].x;
        if (sn.id == MYID)
            s.self_idx = i;
        id2idx[sn.id] = i;
    }

    if (s.self_idx == -1)
    {
        cout << "0\n|ERR:SNAKE_NOT_FOUND\n";
        exit(0);
    }

    // Read chests
    int nc;
    cin >> nc;
    s.chests.resize(nc);
    for (int i = 0; i < nc; i++)
        cin >> s.chests[i].pos.y >> s.chests[i].pos.x >> s.chests[i].score;

    // Read keys
    int nk;
    cin >> nk;
    s.keys.resize(nk);
    for (int i = 0; i < nk; i++)
    {
        cin >> s.keys[i].pos.y >> s.keys[i].pos.x >> s.keys[i].holder_id >> s.keys[i].remaining_time;
        if (s.keys[i].holder_id != -1)
        {
            auto it = id2idx.find(s.keys[i].holder_id);
            if (it != id2idx.end())
                s.snakes[it->second].has_key = true;
        }
    }

    // Read safe zones - CORRECT FORMAT: y_min x_min y_max x_max
    cin >> s.cur.y_min >> s.cur.x_min >> s.cur.y_max >> s.cur.x_max;
    cin >> s.next_tick >> s.next.y_min >> s.next.x_min >> s.next.y_max >> s.next.x_max;
    cin >> s.fin_tick >> s.fin.y_min >> s.fin.x_min >> s.fin.y_max >> s.fin.x_max;
}

// ==================== Helper Functions ====================
inline bool in_bounds(int y, int x)
{
    return (0 <= y && y < H && 0 <= x && x < W);
}

inline bool in_safe_zone(const Safe &z, int y, int x)
{
    return x >= z.x_min && x <= z.x_max && y >= z.y_min && y <= z.y_max;
}

inline bool can_open_shield()
{
    return global_state.self().shield_cd == 0 && global_state.self().score >= SHIELD_COST_THRESHOLD;
}

// ==================== Map Building ====================
array<array<int, W>, H> mp;

static void build_map(const State &s)
{
    // Initialize map
    for (int y = 0; y < H; y++)
    {
        for (int x = 0; x < W; x++)
        {
            mp[y][x] = in_safe_zone(s.cur, y, x) ? 999999 : 0;
        }
    }

    // Place items
    for (const auto &item : s.items)
    {
        if (in_bounds(item.pos.y, item.pos.x) && in_safe_zone(s.cur, item.pos.y, item.pos.x))
        {
            mp[item.pos.y][item.pos.x] = item.type;
        }
    }

    // Place chests (only block if no key)
    for (const auto &chest : s.chests)
    {
        if (in_bounds(chest.pos.y, chest.pos.x) && in_safe_zone(s.cur, chest.pos.y, chest.pos.x))
        {
            mp[chest.pos.y][chest.pos.x] = -5;
        }
    }

    // Place ground keys
    for (const auto &key : s.keys)
    {
        if (key.holder_id == -1 && in_bounds(key.pos.y, key.pos.x) && in_safe_zone(s.cur, key.pos.y, key.pos.x))
        {
            mp[key.pos.y][key.pos.x] = -3;
        }
    }

    // Place snake bodies
    for (const auto &snake : s.snakes)
    {
        for (const auto &body_part : snake.body)
        {
            if (in_bounds(body_part.y, body_part.x) && in_safe_zone(s.cur, body_part.y, body_part.x))
            {
                mp[body_part.y][body_part.x] = snake.id;
            }
        }
    }
}

// Check if position is blocked
inline bool unsafe_to_go(int y, int x, bool has_key = false)
{
    if (!in_bounds(y, x) || mp[y][x] == 0)
        return true;
    int val = mp[y][x];
    if (val == 999999)
        return false;
    if (val > 1000 && val != MYID)
        return true; // Other snake body
    if (val == -5 && !has_key)
        return true; // Chest without key
    return false;
}

inline bool is_enemy_snake(int y, int x)
{
    if (!in_bounds(y, x))
        return false;
    return mp[y][x] > 1000 && mp[y][x] != MYID;
}

inline bool is_trap(int y, int x)
{
    return in_bounds(y, x) && mp[y][x] == -2;
}

// ==================== BFS Pathfinding ====================
struct BFSOut
{
    array<array<int, W>, H> dist;
    array<array<int, W>, H> snake_cost;
    array<array<int, W>, H> parent;
};

static BFSOut bfs_grid(const State &s, int sy, int sx)
{
    BFSOut out;

    for (int y = 0; y < H; y++)
    {
        for (int x = 0; x < W; x++)
        {
            out.dist[y][x] = 1e9;
            out.snake_cost[y][x] = 0;
            out.parent[y][x] = -1;
        }
    }

    priority_queue<tuple<int, int, int, int>, vector<tuple<int, int, int, int>>, greater<>> pq;
    out.dist[sy][sx] = 0;
    out.snake_cost[sy][sx] = 0;
    pq.emplace(0, 0, sy, sx);

    while (!pq.empty())
    {
        auto [total_cost, snake_steps, y, x] = pq.top();
        pq.pop();

        if (total_cost > out.dist[y][x] + out.snake_cost[y][x] * SNAKE_COST_WEIGHT)
            continue;

        for (int k = 0; k < 4; k++)
        {
            int ny = y + DY[k], nx = x + DX[k];

            if (!in_bounds(ny, nx))
                continue;
            if (mp[ny][nx] == -5 && !s.self().has_key)
                continue;

            int new_dist = out.dist[y][x] + 1;
            int new_snake_cost = out.snake_cost[y][x];

            // Snake body or outside safe zone
            if (is_enemy_snake(ny, nx) || !in_safe_zone(s.cur, ny, nx))
            {
                if (s.self().shield_time > 0)
                {
                    new_snake_cost += SNAKE_COST_WITH_SHIELD;
                }
                else if (can_open_shield())
                {
                    new_snake_cost += SNAKE_COST_NO_SHIELD;
                    new_dist += 1;
                }
                else
                {
                    new_snake_cost += 10000;
                }
            }

            // Trap penalty
            int extra_cost = 0;
            if (is_trap(ny, nx))
            {
                extra_cost += TRAP_STEP_COST;
            }

            // Adjacent snake body penalty
            if (!is_enemy_snake(ny, nx))
            {
                int adj_snakes = 0;
                for (int t = 0; t < 4; t++)
                {
                    int ay = ny + DY[t], ax = nx + DX[t];
                    if (is_enemy_snake(ay, ax))
                        adj_snakes++;
                }
                extra_cost += adj_snakes * NEAR_SNAKE_ADJ_PENALTY;
            }

            int new_total_cost = new_dist + new_snake_cost * SNAKE_COST_WEIGHT + extra_cost;

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

// ==================== Target Evaluation ====================
struct Target
{
    int y, x;
    double score;
    int dist;
    int snake_cost;
};

static int min_opponent_distance(const State &s, int y, int x)
{
    int best = 1000000;
    for (const auto &sn : s.snakes)
    {
        if (sn.id == MYID)
            continue;
        const auto &h = sn.head();
        int md = abs(y - h.y) + abs(x - h.x);
        if (md < best)
            best = md;
    }
    return best;
}

// ==================== Survival Strategy ====================
static int survival_strategy(const State &s, int sy, int sx, stringstream &log_ss)
{
    const auto &me = s.self();
    log_ss << "SURVIVAL_MODE:|";

    int bestDir = -1;
    int bestReach = -1;

    // Find safest direction with best reachability
    for (int k = 0; k < 4; k++)
    {
        int ny = sy + DY[k], nx = sx + DX[k];

        // Prevent 180-degree turn
        if (k == (me.dir + 2) % 4)
            continue;

        if (!in_bounds(ny, nx) || !in_safe_zone(s.cur, ny, nx))
            continue;
        if (unsafe_to_go(ny, nx, me.has_key))
            continue;
        if (is_enemy_snake(ny, nx) && me.shield_time == 0)
            continue;

        // Count reachable cells
        int reach = 0;
        for (int t = 0; t < 4; t++)
        {
            int py = ny + DY[t], px = nx + DX[t];
            if (in_bounds(py, px) && in_safe_zone(s.cur, py, px) &&
                !unsafe_to_go(py, px, me.has_key) && !is_enemy_snake(py, px))
            {
                reach++;
            }
        }

        // Penalize dangerous zones and traps
        if (is_trap(ny, nx))
            reach -= 2;

        // Check for enemy heads nearby
        bool danger = false;
        for (const auto &sn : s.snakes)
        {
            if (sn.id == MYID)
                continue;
            auto h = sn.head();
            if (abs(ny - h.y) + abs(nx - h.x) == 1)
            {
                danger = true;
                break;
            }
        }
        if (danger && me.shield_time == 0)
            reach -= 3;

        if (reach > bestReach)
        {
            bestReach = reach;
            bestDir = k;
        }
    }

    if (bestDir != -1)
    {
        log_ss << "SAFE_MOVE:DIR" << bestDir << ",r:" << bestReach << "|";
        return ACT[bestDir];
    }

    // Try shield if available
    if (me.shield_time == 0 && can_open_shield())
    {
        log_ss << "SHIELD_ACTIVATION|";
        return 4;
    }

    // Desperate move - allow traps
    for (int k = 0; k < 4; k++)
    {
        if (k == (me.dir + 2) % 4)
            continue;
        int ny = sy + DY[k], nx = sx + DX[k];
        if (in_bounds(ny, nx) && in_safe_zone(s.cur, ny, nx) &&
            (!is_enemy_snake(ny, nx) || me.shield_time > 0))
        {
            log_ss << "DESPERATE_MOVE:DIR" << k << "|";
            return ACT[k];
        }
    }

    log_ss << "NO_VALID_MOVE|";
    return 0;
}

// ==================== Main Decision Function ====================
struct Choice
{
    int action;
};

static Choice decide(const State &s)
{
    const auto &me = s.self();
    build_map(s);
    const int sy = me.head().y, sx = me.head().x;

    stringstream log_ss;
    log_ss << "T:" << s.current_ticks << "|P:" << sy << "," << sx
           << "|S:" << me.score << "|L:" << me.length << "|";

    // Emergency: Outside safe zone
    if (!in_bounds(sy, sx) || !in_safe_zone(s.cur, sy, sx))
    {
        log_ss << "OUTSIDE_SAFE|";
        if (me.shield_time == 0 && can_open_shield())
        {
            str_info = log_ss.str();
            return {4};
        }

        // Find closest safe zone entry
        int best_dir = -1;
        int min_dist = INT_MAX;
        for (int k = 0; k < 4; k++)
        {
            int ny = sy + DY[k], nx = sx + DX[k];
            if (!in_bounds(ny, nx))
                continue;

            int dist = 0;
            if (in_safe_zone(s.cur, ny, nx))
            {
                dist = 0;
            }
            else
            {
                if (nx < s.cur.x_min)
                    dist += s.cur.x_min - nx;
                else if (nx > s.cur.x_max)
                    dist += nx - s.cur.x_max;
                if (ny < s.cur.y_min)
                    dist += s.cur.y_min - ny;
                else if (ny > s.cur.y_max)
                    dist += ny - s.cur.y_max;
            }

            if (dist < min_dist)
            {
                min_dist = dist;
                best_dir = k;
            }
        }

        if (best_dir != -1)
        {
            log_ss << "RETURN_SAFE:DIR" << best_dir << "|";
            str_info = log_ss.str();
            return {ACT[best_dir]};
        }
    }

    // Head-to-head collision detection
    bool opp_next[H][W] = {};
    for (const auto &sn : s.snakes)
    {
        if (sn.id == MYID)
            continue;
        auto h = sn.head();
        for (int k = 0; k < 4; k++)
        {
            int ny = h.y + DY[k], nx = h.x + DX[k];
            if (in_bounds(ny, nx) && in_safe_zone(s.cur, ny, nx))
            {
                opp_next[ny][nx] = true;
            }
        }
    }

    // BFS from current position
    BFSOut G = bfs_grid(s, sy, sx);

    // Build candidate targets
    vector<Target> candidates;

    // Food items
    for (const auto &item : s.items)
    {
        if (item.type == -2 || item.type == -5)
            continue; // Skip traps and chests
        if (item.type == -3 && me.has_key)
            continue; // Skip keys if already have one

        if (!in_bounds(item.pos.y, item.pos.x) || !in_safe_zone(s.cur, item.pos.y, item.pos.x))
            continue;

        int d = G.dist[item.pos.y][item.pos.x];
        int snake_steps = G.snake_cost[item.pos.y][item.pos.x];
        if (d >= 1e9)
            continue;
        if (item.lifetime != -1 && d > item.lifetime)
            continue;

        double safety_penalty = 1.0 + snake_steps * SNAKE_SAFETY_PENALTY_RATE;
        int d_opp = min_opponent_distance(s, item.pos.y, item.pos.x);
        double contest_factor = (d_opp <= d) ? CONTEST_PENALTY : 1.0;

        double zone_factor = 1.0;
        if (s.next_tick != -1 && (s.current_ticks + d) >= s.next_tick)
        {
            if (!in_safe_zone(s.next, item.pos.y, item.pos.x))
            {
                zone_factor = NEXTZONE_RISK_PENALTY;
            }
        }

        double lifetime_factor = (item.lifetime == -1) ? 1.0 : pow(LIFETIME_SOFT_DECAY, d);
        double score = (item.value * lifetime_factor * contest_factor * zone_factor) /
                       ((d + DISTANCE_OFFSET) * safety_penalty);

        candidates.push_back({item.pos.y, item.pos.x, score, d, snake_steps});
    }

    // Keys (if don't have one)
    if (!me.has_key)
    {
        for (const auto &key : s.keys)
        {
            if (key.holder_id != -1)
                continue;
            if (!in_bounds(key.pos.y, key.pos.x) || !in_safe_zone(s.cur, key.pos.y, key.pos.x))
                continue;

            int d = G.dist[key.pos.y][key.pos.x];
            int snake_steps = G.snake_cost[key.pos.y][key.pos.x];
            if (d >= 1e9)
                continue;

            double safety_penalty = 1.0 + snake_steps * SNAKE_SAFETY_PENALTY_RATE;
            double score = KEY_VALUE / ((d + DISTANCE_OFFSET) * safety_penalty);
            candidates.push_back({key.pos.y, key.pos.x, score, d, snake_steps});
        }
    }

    // Chests (if have key)
    if (me.has_key)
    {
        for (const auto &chest : s.chests)
        {
            if (!in_bounds(chest.pos.y, chest.pos.x) || !in_safe_zone(s.cur, chest.pos.y, chest.pos.x))
                continue;

            int d = G.dist[chest.pos.y][chest.pos.x];
            int snake_steps = G.snake_cost[chest.pos.y][chest.pos.x];
            if (d >= 1e9)
                continue;

            double safety_penalty = 1.0 + snake_steps * SNAKE_SAFETY_PENALTY_RATE;
            double chest_value = (chest.score > 0) ? chest.score * CHEST_SCORE_MULTIPLIER : 60.0;
            double score = chest_value / ((d + DISTANCE_OFFSET) * safety_penalty);
            candidates.push_back({chest.pos.y, chest.pos.x, score, d, snake_steps});
        }
    }

    log_ss << "CAND:" << candidates.size() << "|";

    // No targets - survival mode
    if (candidates.empty())
    {
        int choice = survival_strategy(s, sy, sx, log_ss);
        str_info = log_ss.str();
        return {choice};
    }

    // Sort by score
    sort(candidates.begin(), candidates.end(), [](const Target &a, const Target &b)
         { return a.score > b.score || (a.score == b.score && a.dist < b.dist); });

    // Try to reach best target
    for (const auto &target : candidates)
    {
        // Backtrack path
        int ty = target.y, tx = target.x;
        if (G.parent[ty][tx] == -1)
            continue;

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
        for (int k = 0; k < 4; k++)
        {
            if (sy + DY[k] == cy && sx + DX[k] == cx)
            {
                dir = k;
                break;
            }
        }

        if (dir == -1)
            continue;

        // Prevent 180-degree turn
        if (dir == (me.dir + 2) % 4)
            continue;

        int ny = sy + DY[dir], nx = sx + DX[dir];

        // Head-to-head collision check
        if (opp_next[ny][nx] && me.shield_time == 0)
        {
            // Try alternative direction
            bool found_alt = false;
            for (int k = 0; k < 4; k++)
            {
                if (k == (me.dir + 2) % 4)
                    continue;
                int alt_y = sy + DY[k], alt_x = sx + DX[k];
                if (!in_bounds(alt_y, alt_x) || !in_safe_zone(s.cur, alt_y, alt_x))
                    continue;
                if (unsafe_to_go(alt_y, alt_x, me.has_key))
                    continue;
                if (!opp_next[alt_y][alt_x])
                {
                    dir = k;
                    ny = alt_y;
                    nx = alt_x;
                    found_alt = true;
                    break;
                }
            }
            if (!found_alt && can_open_shield())
            {
                log_ss << "SHIELD_H2H|";
                str_info = log_ss.str();
                return {4};
            }
            if (!found_alt)
                continue;
        }

        // Check if need shield for snake body
        if (is_enemy_snake(ny, nx) && me.shield_time == 0)
        {
            if (can_open_shield() && target.snake_cost > 0)
            {
                log_ss << "SHIELD_SNAKE|";
                str_info = log_ss.str();
                return {4};
            }
            continue;
        }

        log_ss << "TARGET:(" << target.y << "," << target.x << ")sc:" << (int)(target.score * 100)
               << ",d:" << target.dist << ",DIR:" << dir << "|";
        str_info = log_ss.str();
        return {ACT[dir]};
    }

    // Fallback to survival
    int choice = survival_strategy(s, sy, sx, log_ss);
    str_info = log_ss.str();
    return {choice};
}

// ==================== Main Entry ====================
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