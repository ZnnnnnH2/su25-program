#include <iostream>
using namespace std;

// 简单测试方向和位置
static const int DX[4] = {-1, 0, 1, 0}; // x方向偏移：左、上、右、下
static const int DY[4] = {0, -1, 0, 1}; // y方向偏移：左、上、右、下

int main()
{
    int sy = 6, sx = 16; // 蛇头位置
    int ty = 6, tx = 15; // 目标位置

    cout << "Snake head: (" << sy << "," << sx << ")" << endl;
    cout << "Target: (" << ty << "," << tx << ")" << endl;

    // 计算曼哈顿距离
    int dist = abs(sy - ty) + abs(sx - tx);
    cout << "Manhattan distance: " << dist << endl;

    // 检查各个方向
    for (int k = 0; k < 4; k++)
    {
        int ny = sy + DY[k];
        int nx = sx + DX[k];
        cout << "Direction " << k << ": (" << ny << "," << nx << ")";
        if (ny == ty && nx == tx)
        {
            cout << " -> REACHES TARGET!";
        }
        cout << endl;
    }

    // 蛇当前方向是0（左），目标在左边，应该可以直接到达
    int target_dir = -1;
    for (int k = 0; k < 4; k++)
    {
        int ny = sy + DY[k];
        int nx = sx + DX[k];
        if (ny == ty && nx == tx)
        {
            target_dir = k;
            break;
        }
    }

    cout << "Direct direction to target: " << target_dir << endl;

    return 0;
}
