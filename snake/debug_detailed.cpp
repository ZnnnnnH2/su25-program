#include <iostream>
#include <vector>
#include <fstream>
using namespace std;

int main()
{
    // 创建地图 40x30
    vector<vector<char>> map(30, vector<char>(40, '.'));

    // 从input.in读取蛇的位置
    ifstream fin("input.in");

    int remaining_ticks;
    fin >> remaining_ticks;

    int m;
    fin >> m;
    for (int i = 0; i < m; i++)
    {
        int y, x, type, lifetime;
        fin >> y >> x >> type >> lifetime;
        if (type >= 1)
            map[y][x] = ('0' + type); // 食物显示为数字
        else if (type == -1)
            map[y][x] = 'G'; // 成长食物
        else if (type == -2)
            map[y][x] = 'T'; // 陷阱
        else if (type == -3)
            map[y][x] = 'K'; // 钥匙
        else if (type == -5)
            map[y][x] = 'C'; // 宝箱
    }

    int ns;
    fin >> ns;
    for (int i = 0; i < ns; i++)
    {
        int id, length, score, dir, shield_cd, shield_time;
        fin >> id >> length >> score >> dir >> shield_cd >> shield_time;

        cout << "蛇 ID " << id << " (长度:" << length << ", 方向:" << dir << "):" << endl;

        for (int j = 0; j < length; j++)
        {
            int y, x;
            fin >> y >> x;
            cout << "  [" << j << "] (" << y << "," << x << ")";
            if (j == 0)
                cout << " <-- 头部";
            cout << endl;

            if (id == 2024201540)
            { // 我的蛇
                if (j == 0)
                    map[y][x] = 'H'; // 头部
                else
                    map[y][x] = ('0' + (j % 10)); // 身体用数字标记顺序
            }
            else
            {
                if (j == 0)
                    map[y][x] = 'E'; // 敌人头部
                else
                    map[y][x] = 'B'; // 敌人身体
            }
        }
        cout << endl;
    }

    // 输出地图，重点关注我的蛇头周围
    cout << "地图区域 (7-13, 22-29):" << endl;
    cout << "   ";
    for (int x = 22; x <= 29; x++)
    {
        cout << (x % 10);
    }
    cout << endl;

    for (int y = 7; y <= 13; y++)
    {
        cout << (y < 10 ? " " : "") << y << " ";
        for (int x = 22; x <= 29; x++)
        {
            cout << map[y][x];
        }
        cout << endl;
    }

    cout << "\n图例: H=我的头, 0-9=我的身体(按顺序), E=敌人头, B=敌人身体" << endl;
    cout << "      数字=食物价值, G=成长食物, T=陷阱, K=钥匙, C=宝箱, .=空地" << endl;

    // 分析可能的移动方向
    cout << "\n移动方向分析:" << endl;
    cout << "当前头部位置: (10,25)" << endl;
    cout << "0=左 (10,24): " << map[10][24] << endl;
    cout << "1=上 (9,25): " << map[9][25] << endl;
    cout << "2=右 (10,26): " << map[10][26] << endl;
    cout << "3=下 (11,25): " << map[11][25] << endl;

    return 0;
}
