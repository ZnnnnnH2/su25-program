#include <chrono>
#include <iostream>
#include <random>
#include <unordered_map>
#include <vector>

using namespace std;

// 常量
constexpr int MAXN = 40;
constexpr int MAXM = 30;
constexpr int MAX_TICKS = 256;
constexpr int MYID = 2024201540; // 此处替换为你的学号！

struct Point
{
    int y, x;
};

struct Item
{
    Point pos;
    int value;
    int lifetime;
};

struct Snake
{
    int id;
    int length;
    int score;
    int direction;
    int shield_cd;
    int shield_time;
    bool has_key;
    vector<Point> body;

    const Point &get_head() const { return body.front(); }
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

struct SafeZoneBounds
{
    int x_min, y_min, x_max, y_max;
};

struct GameState
{
    int remaining_ticks;
    vector<Item> items;
    vector<Snake> snakes;
    vector<Chest> chests;
    vector<Key> keys;
    SafeZoneBounds current_safe_zone;
    int next_shrink_tick;
    SafeZoneBounds next_safe_zone;
    int final_shrink_tick;
    SafeZoneBounds final_safe_zone;

    int self_idx;

    const Snake &get_self() const { return snakes[self_idx]; }
};

void read_game_state(GameState &s)
{
    cin >> s.remaining_ticks;

    int item_count;
    cin >> item_count;
    s.items.resize(item_count);
    for (int i = 0; i < item_count; ++i)
    {
        cin >> s.items[i].pos.y >> s.items[i].pos.x >>
            s.items[i].value >> s.items[i].lifetime;
    }

    int snake_count;
    cin >> snake_count;
    s.snakes.resize(snake_count);
    unordered_map<int, int> id2idx;
    id2idx.reserve(snake_count * 2);

    for (int i = 0; i < snake_count; ++i)
    {
        auto &sn = s.snakes[i];
        cin >> sn.id >> sn.length >> sn.score >> sn.direction >> sn.shield_cd >>
            sn.shield_time;
        sn.body.resize(sn.length);
        for (int j = 0; j < sn.length; ++j)
        {
            cin >> sn.body[j].y >> sn.body[j].x;
        }
        if (sn.id == MYID)
            s.self_idx = i;
        id2idx[sn.id] = i;
    }

    int chest_count;
    cin >> chest_count;
    s.chests.resize(chest_count);
    for (int i = 0; i < chest_count; ++i)
    {
        cin >> s.chests[i].pos.y >> s.chests[i].pos.x >>
            s.chests[i].score;
    }

    int key_count;
    cin >> key_count;
    s.keys.resize(key_count);
    for (int i = 0; i < key_count; ++i)
    {
        auto &key = s.keys[i];
        cin >> key.pos.y >> key.pos.x >> key.holder_id >> key.remaining_time;
        if (key.holder_id != -1)
        {
            auto it = id2idx.find(key.holder_id);
            if (it != id2idx.end())
            {
                s.snakes[it->second].has_key = true;
            }
        }
    }

    cin >> s.current_safe_zone.x_min >> s.current_safe_zone.y_min >>
        s.current_safe_zone.x_max >> s.current_safe_zone.y_max;
    cin >> s.next_shrink_tick >> s.next_safe_zone.x_min >>
        s.next_safe_zone.y_min >> s.next_safe_zone.x_max >>
        s.next_safe_zone.y_max;
    cin >> s.final_shrink_tick >> s.final_safe_zone.x_min >>
        s.final_safe_zone.y_min >> s.final_safe_zone.x_max >>
        s.final_safe_zone.y_max;

    // 如果上一个 tick 往 Memory 里写入了内容，在这里读取，注意处理第一个 tick
    // 的情况 if (s.remaining_ticks < MAX_TICKS) {
    //     // 处理 Memory 读取
    // }
}

int main()
{
    // 读取当前 tick 的所有游戏状态
    GameState current_state;
    read_game_state(current_state);

    // 随机选择一个方向作为决策
    mt19937 rng(chrono::steady_clock::now().time_since_epoch().count());
    uniform_int_distribution<int> dist(0, 3);
    int decision = dist(rng);
    cout << decision << endl;
    // C++ 23 也可使用 std::print
    // 如果需要写入 Memory，在此处写入

    return 0;
}
