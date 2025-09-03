#include <iostream>
#include <vector>
#include <sstream>
using namespace std;

// 简化的测试，只检查输入读取和基本逻辑

struct Point
{
    int y, x;
};
struct Safe
{
    int x_min, y_min, x_max, y_max;
};

bool in_safe_zone(const Safe &z, int y, int x)
{
    return x >= z.x_min && x <= z.x_max && y >= z.y_min && y <= z.y_max;
}

int main()
{
    // 读取剩余回合数
    int remaining_ticks;
    if (!(cin >> remaining_ticks))
    {
        cout << "Failed to read remaining_ticks" << endl;
        return 0;
    }
    cout << "Remaining ticks: " << remaining_ticks << endl;

    // 读取物品数量
    int m;
    cin >> m;
    cout << "Items count: " << m << endl;

    // 跳过所有物品
    for (int i = 0; i < m; i++)
    {
        int y, x, type, value;
        cin >> y >> x >> type >> value;
    }

    // 读取蛇数量
    int ns;
    cin >> ns;
    cout << "Snakes count: " << ns << endl;

    Point my_head;
    bool found_myself = false;

    // 读取所有蛇
    for (int i = 0; i < ns; i++)
    {
        int id, length, score, dir, shield_cd, shield_time;
        cin >> id >> length >> score >> dir >> shield_cd >> shield_time;

        if (id == 2024201540)
        {
            cout << "Found myself: length=" << length << ", score=" << score << ", dir=" << dir << endl;
            found_myself = true;

            // 读取第一个身体部分（蛇头）
            cin >> my_head.y >> my_head.x;
            cout << "My head: (" << my_head.y << "," << my_head.x << ")" << endl;

            // 跳过剩余身体
            for (int j = 1; j < length; j++)
            {
                int y, x;
                cin >> y >> x;
            }
        }
        else
        {
            // 跳过其他蛇的身体
            for (int j = 0; j < length; j++)
            {
                int y, x;
                cin >> y >> x;
            }
        }
    }

    if (!found_myself)
    {
        cout << "ERROR: Could not find myself in snakes list!" << endl;
        return 0;
    }

    // 跳过宝箱
    int nc;
    cin >> nc;
    for (int i = 0; i < nc; i++)
    {
        int y, x, score;
        cin >> y >> x >> score;
    }

    // 跳过钥匙
    int nk;
    cin >> nk;
    for (int i = 0; i < nk; i++)
    {
        int y, x, holder_id, remaining_time;
        cin >> y >> x >> holder_id >> remaining_time;
    }

    // 读取安全区
    Safe cur;
    cin >> cur.x_min >> cur.y_min >> cur.x_max >> cur.y_max;
    cout << "Current safe zone: [" << cur.x_min << "," << cur.x_max << "] x [" << cur.y_min << "," << cur.y_max << "]" << endl;

    // 检查是否在安全区内
    bool in_safe = in_safe_zone(cur, my_head.y, my_head.x);
    cout << "Am I in safe zone? " << (in_safe ? "YES" : "NO") << endl;

    if (!in_safe)
    {
        cout << "EMERGENCY: Outside safe zone!" << endl;
        return 0; // 这就是为什么输出0的原因
    }

    cout << "Normal processing should continue..." << endl;
    return 1;
}
