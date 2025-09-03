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
            map[y][x] = 'F'; // 食物
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

        for (int j = 0; j < length; j++)
        {
            int y, x;
            fin >> y >> x;
            if (id == 2024201540)
            { // 我的蛇
                if (j == 0)
                    map[y][x] = 'H'; // 头部
                else
                    map[y][x] = 'M'; // 我的身体
            }
            else
            {
                if (j == 0)
                    map[y][x] = 'E'; // 敌人头部
                else
                    map[y][x] = 'B'; // 敌人身体
            }
        }
    }

    // 输出地图，重点关注我的蛇头周围
    cout << "Map around my snake head (10,25):" << endl;
    for (int y = 8; y <= 12; y++)
    {
        for (int x = 23; x <= 27; x++)
        {
            cout << map[y][x];
        }
        cout << " (y=" << y << ")" << endl;
    }

    cout << "\nLegend: H=我的头, M=我的身体, E=敌人头, B=敌人身体, F=食物, G=成长食物, T=陷阱, K=钥匙, C=宝箱, .=空地" << endl;

    // 检查向左移动的安全性
    cout << "\n检查向左移动 (10,25) -> (10,24): ";
    if (map[10][24] == '.' || map[10][24] == 'F' || map[10][24] == 'G' || map[10][24] == 'K' || map[10][24] == 'C')
    {
        cout << "安全 (" << map[10][24] << ")" << endl;
    }
    else
    {
        cout << "危险！(" << map[10][24] << ")" << endl;
    }

    return 0;
}
