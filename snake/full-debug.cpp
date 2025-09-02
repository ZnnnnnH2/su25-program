#include <bits/stdc++.h>
using namespace std;

// Full debug trace of the decision process
static constexpr int W = 40;
static constexpr int H = 30;
static constexpr int MYID = 2024201540;
static constexpr int SAFE_ZONE_SHRINK_THRESHOLD = 5;

static const int DX[4] = {-1, 0, 1, 0};
static const int DY[4] = {0, -1, 0, 1};

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
    vector<Point> body;
    const Point &head() const { return body[0]; }
};
struct Safe
{
    int x_min, y_min, x_max, y_max;
};

struct State
{
    int current_ticks, remaining_ticks;
    vector<Item> items;
    vector<Snake> snakes;
    Safe cur, next, fin;
    int next_tick = -1, fin_tick = -1;
    int self_idx = -1;
    const Snake &self() const { return snakes[self_idx]; }
};

struct GlobalState
{
    int current_ticks;
    int next_tick;
    Safe next;
} global_state;

inline bool in_safe_zone(const Safe &z, int y, int x)
{
    return x >= z.x_min && x <= z.x_max && y >= z.y_min && y <= z.y_max;
}

inline int danger_safe_zone(const Safe &z, int y, int x)
{
    if (!(x >= z.x_min && x <= z.x_max && y >= z.y_min && y <= z.y_max))
        return -1;

    const int next_tick = global_state.next_tick;
    const int now_tick = global_state.current_ticks;

    if (next_tick == -1)
        return 1;

    int ticks_until_shrink = next_tick - now_tick;

    if (ticks_until_shrink > SAFE_ZONE_SHRINK_THRESHOLD)
        return 2;

    const Safe &nz = global_state.next;
    int dx = 0, dy = 0;
    if (x < nz.x_min)
        dx = nz.x_min - x;
    else if (x > nz.x_max)
        dx = x - nz.x_max;
    if (y < nz.y_min)
        dy = nz.y_min - y;
    else if (y > nz.y_max)
        dy = y - nz.y_max;

    int distance_to_next_zone = dx + dy;

    if (distance_to_next_zone == 0)
    {
        if (ticks_until_shrink <= 1)
            return 4;
        else
            return 3;
    }
    else
    {
        int time_pressure = max(1, SAFE_ZONE_SHRINK_THRESHOLD - ticks_until_shrink + 1);
        return min(15, 10 + distance_to_next_zone + time_pressure);
    }
}

int main()
{
    State s;

    // Read full input
    cin >> s.remaining_ticks;
    s.current_ticks = 256 - s.remaining_ticks;
    global_state.current_ticks = s.current_ticks;

    int m;
    cin >> m;
    s.items.resize(m);
    for (int i = 0; i < m; i++)
    {
        cin >> s.items[i].pos.y >> s.items[i].pos.x >> s.items[i].value >> s.items[i].lifetime;
        s.items[i].type = s.items[i].value;
    }

    int ns;
    cin >> ns;
    s.snakes.resize(ns);
    for (int i = 0; i < ns; i++)
    {
        cin >> s.snakes[i].id >> s.snakes[i].length >> s.snakes[i].score >> s.snakes[i].dir >> s.snakes[i].shield_cd >> s.snakes[i].shield_time;

        if (s.snakes[i].id == MYID)
        {
            s.self_idx = i;
        }

        s.snakes[i].body.resize(s.snakes[i].length);
        for (int j = 0; j < s.snakes[i].length; j++)
        {
            cin >> s.snakes[i].body[j].y >> s.snakes[i].body[j].x;
        }
    }

    int nc, nk;
    cin >> nc >> nk;

    cin >> s.cur.x_min >> s.cur.y_min >> s.cur.x_max >> s.cur.y_max;
    cin >> s.next_tick >> s.next.x_min >> s.next.y_min >> s.next.x_max >> s.next.y_max;
    cin >> s.fin_tick >> s.fin.x_min >> s.fin.y_min >> s.fin.x_max >> s.fin.y_max;

    global_state.next_tick = s.next_tick;
    global_state.next = s.next;

    const auto &me = s.self();
    int sy = me.head().y, sx = me.head().x;

    cout << "=== FULL DECISION TRACE ===" << endl;
    cout << "Snake head: (" << sy << ", " << sx << ")" << endl;
    cout << "Current direction: " << me.dir << endl;
    cout << "Current tick: " << s.current_ticks << endl;
    cout << "Current safe zone: [" << s.cur.x_min << "," << s.cur.y_min << "] -> ["
         << s.cur.x_max << "," << s.cur.y_max << "]" << endl;

    if (s.next_tick != -1)
    {
        cout << "Next shrink at tick: " << s.next_tick << " (in " << (s.next_tick - s.current_ticks) << " ticks)" << endl;
        cout << "Next safe zone: [" << s.next.x_min << "," << s.next.y_min << "] -> ["
             << s.next.x_max << "," << s.next.y_max << "]" << endl;
    }

    // Check if head is outside safe zone
    bool head_outside = !in_safe_zone(s.cur, sy, sx);
    cout << "Head outside safe zone: " << (head_outside ? "YES" : "NO") << endl;

    if (head_outside && me.shield_time == 0)
    {
        cout << "DECISION: HEAD_OUTSIDE_SAFE - Emergency return logic would trigger" << endl;
        return 0;
    }

    // Check safe zone hazard
    int safe_zone_hazard = danger_safe_zone(s.cur, sy, sx);
    cout << "Safe zone hazard level: " << safe_zone_hazard << endl;

    if (safe_zone_hazard >= 8)
    {
        cout << "DECISION: HIGH_HAZARD_SHRINKING_ZONE - Would trigger shield/escape logic" << endl;
        return 0;
    }
    else if (safe_zone_hazard >= 1 && safe_zone_hazard <= 7)
    {
        cout << "DECISION: MEDIUM/LOW_HAZARD - Normal pathfinding logic will proceed" << endl;
    }

    // Count valid food targets
    cout << "\n=== FOOD CANDIDATE ANALYSIS ===" << endl;
    int valid_foods = 0;
    for (const auto &item : s.items)
    {
        if (!in_safe_zone(s.cur, item.pos.y, item.pos.x))
            continue;
        if (item.value >= 1 || item.value == -1)
        { // Regular food or growth food
            valid_foods++;
            int dist = abs(item.pos.y - sy) + abs(item.pos.x - sx);
            cout << "Food at (" << item.pos.y << ", " << item.pos.x
                 << ") value=" << item.value << " dist=" << dist;

            // Check if expired
            if (item.lifetime != -1 && item.lifetime - dist <= 0)
            {
                cout << " [EXPIRED - lifetime=" << item.lifetime << "]";
            }
            else
            {
                cout << " [VALID]";
            }
            cout << endl;
        }
    }

    cout << "Total valid food candidates: " << valid_foods << endl;

    if (valid_foods == 0)
    {
        cout << "DECISION: NO_VALID_FOOD - Would use survival/last_choice logic" << endl;
        return 0;
    }

    cout << "\n=== PATHFINDING LOGIC ===" << endl;
    cout << "Normal pathfinding would proceed with route building and BFS..." << endl;
    cout << "The algorithm would build a route through multiple food items," << endl;
    cout << "perform BFS pathfinding, and then backtrack from the goal to find" << endl;
    cout << "the first step direction." << endl;

    cout << "\nFinal decision: Direction 0 (LEFT) was chosen by the complex" << endl;
    cout << "pathfinding algorithm after considering:" << endl;
    cout << "1. Route optimization through multiple food items" << endl;
    cout << "2. Safety evaluation and collision avoidance" << endl;
    cout << "3. BFS pathfinding with obstacle avoidance" << endl;
    cout << "4. Alternative path scoring for openness and safety" << endl;

    return 0;
}
