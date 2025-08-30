#include <chrono>        // 时间相关操作，用于随机数种子
#include <iostream>      // 标准输入输出
#include <random>        // 随机数生成
#include <unordered_map> // 哈希表，用于ID到索引的映射
#include <vector>        // 动态数组

using namespace std;

// ==================== 游戏常量定义 ====================
constexpr int MAXN = 40;         // 游戏地图最大行数
constexpr int MAXM = 30;         // 游戏地图最大列数
constexpr int MAX_TICKS = 256;   // 游戏最大回合数
constexpr int MYID = 2024201540; // 此处替换为你的学号！

// ==================== 数据结构定义 ====================

/**
 * 坐标点结构
 * 用于表示游戏中的位置坐标
 */
struct Point
{
    int y, x; // y: 行坐标, x: 列坐标
};

/**
 * 游戏物品结构
 * 代表地图上可以被蛇吃掉的物品
 */
struct Item
{
    Point pos;    // 物品位置
    int value;    // 物品价值（分数）
    int lifetime; // 物品剩余生存时间
};

/**
 * 蛇的结构
 * 包含蛇的所有状态信息
 */
struct Snake
{
    int id;             // 蛇的唯一标识符
    int length;         // 蛇的长度
    int score;          // 蛇的当前分数
    int direction;      // 蛇的移动方向 (0:上, 1:右, 2:下, 3:左)
    int shield_cd;      // 护盾冷却时间
    int shield_time;    // 护盾持续时间
    bool has_key;       // 是否持有钥匙
    vector<Point> body; // 蛇身体的所有节点坐标

    // 获取蛇头位置的便捷方法
    const Point &get_head() const { return body.front(); }
};

/**
 * 宝箱结构
 * 需要钥匙才能开启的高分物品
 */
struct Chest
{
    Point pos; // 宝箱位置
    int score; // 宝箱分数
};

/**
 * 钥匙结构
 * 用于开启宝箱的特殊物品
 */
struct Key
{
    Point pos;          // 钥匙位置
    int holder_id;      // 持有者ID (-1表示无人持有)
    int remaining_time; // 钥匙剩余有效时间
};

/**
 * 安全区域边界
 * 定义游戏安全区域的范围
 */
struct SafeZoneBounds
{
    int x_min, y_min, x_max, y_max; // 安全区域的边界坐标
};

/**
 * 游戏状态结构
 * 包含当前游戏的完整状态信息
 */
struct GameState
{
    int remaining_ticks;              // 游戏剩余回合数
    vector<Item> items;               // 地图上的所有物品
    vector<Snake> snakes;             // 游戏中的所有蛇
    vector<Chest> chests;             // 地图上的所有宝箱
    vector<Key> keys;                 // 地图上的所有钥匙
    SafeZoneBounds current_safe_zone; // 当前安全区域范围
    int next_shrink_tick;             // 下次安全区缩小的回合
    SafeZoneBounds next_safe_zone;    // 下次缩小后的安全区域
    int final_shrink_tick;            // 最终缩小的回合
    SafeZoneBounds final_safe_zone;   // 最终安全区域

    int self_idx; // 自己蛇在snakes数组中的索引

    // 获取自己蛇的便捷方法
    const Snake &get_self() const { return snakes[self_idx]; }
};

// ==================== 游戏状态读取函数 ====================

/**
 * 从标准输入读取当前游戏状态
 *
 * 输入格式：
 * 1. 剩余回合数
 * 2. 物品信息：数量 + 每个物品的位置、价值、生存时间
 * 3. 蛇信息：数量 + 每条蛇的详细信息和身体坐标
 * 4. 宝箱信息：数量 + 每个宝箱的位置和分数
 * 5. 钥匙信息：数量 + 每个钥匙的位置、持有者、剩余时间
 * 6. 安全区域信息：当前、下次、最终的安全区域边界
 *
 * @param s 游戏状态结构的引用，用于存储读取到的数据
 */
void read_game_state(GameState &s)
{
    // 读取剩余回合数
    cin >> s.remaining_ticks;

    // ========== 读取物品信息 ==========
    int item_count;
    cin >> item_count;
    s.items.resize(item_count);
    for (int i = 0; i < item_count; ++i)
    {
        // 读取每个物品的位置、价值和生存时间
        cin >> s.items[i].pos.y >> s.items[i].pos.x >>
            s.items[i].value >> s.items[i].lifetime;
    }

    // ========== 读取蛇信息 ==========
    int snake_count;
    cin >> snake_count;
    s.snakes.resize(snake_count);
    // 创建ID到索引的映射表，用于快速查找蛇
    unordered_map<int, int> id2idx;
    id2idx.reserve(snake_count * 2);

    for (int i = 0; i < snake_count; ++i)
    {
        auto &sn = s.snakes[i];
        // 读取蛇的基本信息
        cin >> sn.id >> sn.length >> sn.score >> sn.direction >> sn.shield_cd >>
            sn.shield_time;
        // 读取蛇身体的所有坐标
        sn.body.resize(sn.length);
        for (int j = 0; j < sn.length; ++j)
        {
            cin >> sn.body[j].y >> sn.body[j].x;
        }
        // 如果是自己的蛇，记录索引
        if (sn.id == MYID)
            s.self_idx = i;
        // 建立ID到索引的映射
        id2idx[sn.id] = i;
    }

    // ========== 读取宝箱信息 ==========
    int chest_count;
    cin >> chest_count;
    s.chests.resize(chest_count);
    for (int i = 0; i < chest_count; ++i)
    {
        // 读取每个宝箱的位置和分数
        cin >> s.chests[i].pos.y >> s.chests[i].pos.x >>
            s.chests[i].score;
    }

    // ========== 读取钥匙信息 ==========
    int key_count;
    cin >> key_count;
    s.keys.resize(key_count);
    for (int i = 0; i < key_count; ++i)
    {
        auto &key = s.keys[i];
        // 读取钥匙的位置、持有者和剩余时间
        cin >> key.pos.y >> key.pos.x >> key.holder_id >> key.remaining_time;
        // 如果钥匙被某条蛇持有，标记该蛇有钥匙
        if (key.holder_id != -1)
        {
            auto it = id2idx.find(key.holder_id);
            if (it != id2idx.end())
            {
                s.snakes[it->second].has_key = true;
            }
        }
    }

    // ========== 读取安全区域信息 ==========
    // 当前安全区域边界
    cin >> s.current_safe_zone.x_min >> s.current_safe_zone.y_min >>
        s.current_safe_zone.x_max >> s.current_safe_zone.y_max;
    // 下次缩小的回合和缩小后的安全区域
    cin >> s.next_shrink_tick >> s.next_safe_zone.x_min >>
        s.next_safe_zone.y_min >> s.next_safe_zone.x_max >>
        s.next_safe_zone.y_max;
    // 最终缩小的回合和最终安全区域
    cin >> s.final_shrink_tick >> s.final_safe_zone.x_min >>
        s.final_safe_zone.y_min >> s.final_safe_zone.x_max >>
        s.final_safe_zone.y_max;

    // ========== 内存读取 (预留功能) ==========
    // 如果上一个 tick 往 Memory 里写入了内容，在这里读取
    // 注意处理第一个 tick 的情况
    // if (s.remaining_ticks < MAX_TICKS) {
    //     // 处理 Memory 读取
    // }
}

// ==================== 主程序入口 ====================

/**
 * 主函数 - 程序入口点
 *
 * 程序流程：
 * 1. 读取当前回合的游戏状态
 * 2. 基于游戏状态进行决策计算
 * 3. 输出移动方向决策
 * 4. (可选) 写入内存数据供下回合使用
 *
 * 移动方向编码：
 * 0 - 向上移动
 * 1 - 向右移动
 * 2 - 向下移动
 * 3 - 向左移动
 *
 * @return 程序退出状态码
 */
int main()
{
    // ========== 读取游戏状态 ==========
    // 从标准输入读取当前 tick 的所有游戏状态
    GameState current_state;
    read_game_state(current_state);

    // ========== AI决策逻辑 ==========
    // 目前使用简单的随机策略作为示例
    // TODO: 在这里实现智能的决策算法

    // 初始化随机数生成器（使用当前时间作为种子）
    mt19937 rng(chrono::steady_clock::now().time_since_epoch().count());
    uniform_int_distribution<int> dist(0, 3); // 生成0-3的随机数
    int decision = dist(rng);                 // 随机选择一个移动方向

    // ========== 输出决策结果 ==========
    cout << decision << endl; // 输出移动方向到标准输出
    // 注意：C++23 也可使用 std::print 替代 cout

    // ========== 内存写入 (预留功能) ==========
    // 如果需要在下一回合使用某些数据，可以在此处写入内存
    // 例如：记录路径、敌人位置、策略状态等

    return 0; // 程序正常结束
}
