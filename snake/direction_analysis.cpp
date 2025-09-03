#include <iostream>
using namespace std;

static const int DX[4] = {-1, 0, 1, 0}; // x方向偏移：左、上、右、下
static const int DY[4] = {0, -1, 0, 1}; // y方向偏移：左、上、右、下

int main()
{
    // 测试从(6,16)移动到(6,15)需要什么方向
    int from_y = 6, from_x = 16;
    int to_y = 6, to_x = 15;

    cout << "From (" << from_y << "," << from_x << ") to (" << to_y << "," << to_x << ")" << endl;

    for (int k = 0; k < 4; k++)
    {
        int ny = from_y + DY[k];
        int nx = from_x + DX[k];
        cout << "Direction " << k << ": (" << from_y << "," << from_x << ") -> (" << ny << "," << nx << ")";
        if (ny == to_y && nx == to_x)
        {
            cout << " <- CORRECT DIRECTION";
        }
        cout << endl;
    }

    // 测试反向：从(6,15)回到(6,16)需要什么方向
    cout << "\nReverse: From (" << to_y << "," << to_x << ") back to (" << from_y << "," << from_x << ")" << endl;

    for (int k = 0; k < 4; k++)
    {
        int ny = to_y + DY[k];
        int nx = to_x + DX[k];
        cout << "Direction " << k << ": (" << to_y << "," << to_x << ") -> (" << ny << "," << nx << ")";
        if (ny == from_y && nx == from_x)
        {
            cout << " <- REVERSE DIRECTION";
        }
        cout << endl;
    }

    return 0;
}
