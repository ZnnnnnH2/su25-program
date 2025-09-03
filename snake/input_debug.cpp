#include <iostream>
#include <vector>
using namespace std;

static constexpr int MYID = 2024201540;

int main()
{
    // 读取剩余回合数
    int remaining_ticks;
    if (!(cin >> remaining_ticks))
    {
        cout << "Failed to read remaining_ticks, exiting with 0" << endl;
        cout << "0" << endl;
        return 0;
    }
    cout << "Successfully read remaining_ticks: " << remaining_ticks << endl;

    // 读取物品
    int m;
    cin >> m;
    cout << "Items count: " << m << endl;

    for (int i = 0; i < m; i++)
    {
        int y, x, type, lifetime;
        cin >> y >> x >> type >> lifetime;
        cout << "Item " << i << ": (" << y << "," << x << ") type=" << type << " lifetime=" << lifetime << endl;
    }

    // 读取蛇
    int ns;
    cin >> ns;
    cout << "Snakes count: " << ns << endl;

    int self_idx = -1;

    for (int i = 0; i < ns; i++)
    {
        int id, length, score, dir, shield_cd, shield_time;
        cin >> id >> length >> score >> dir >> shield_cd >> shield_time;
        cout << "Snake " << i << ": ID=" << id << " length=" << length << " score=" << score << " dir=" << dir << endl;

        if (id == MYID)
        {
            self_idx = i;
            cout << "Found myself at index " << i << endl;
        }

        // 读取身体
        for (int j = 0; j < length; j++)
        {
            int y, x;
            cin >> y >> x;
            if (j == 0)
            {
                cout << "  Head at (" << y << "," << x << ")" << endl;
            }
        }
    }

    if (self_idx == -1)
    {
        cout << "ERROR: SNAKE_NOT_FOUND, outputting 0 and exiting" << endl;
        cout << "0" << endl;
        cout << "|ERR:SNAKE_NOT_FOUND" << endl;
        return 0;
    }

    cout << "Successfully found myself, continuing..." << endl;
    return 1;
}
