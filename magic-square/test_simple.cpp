#include <algorithm>
#include <iostream>
#include <vector>
#include <unordered_set>
#include <cstdio>
#include <array>

#define SIZE 3

typedef std::array<std::array<char, SIZE>, SIZE> Square;
typedef std::array<char, SIZE> Line;

int reflaction[200];

class TestMagicSquare
{
private:
    Square magicSquare[6]; // 0: back, 1: down, 2: front, 3: left, 4: right, 5: up

public:
    TestMagicSquare()
    {
        // 初始化一个简单的测试立方体，每个面都有唯一标识
        char faceChars[] = {'B', 'D', 'F', 'L', 'R', 'U'};
        for (int face = 0; face < 6; face++)
        {
            for (int i = 0; i < SIZE; i++)
            {
                for (int j = 0; j < SIZE; j++)
                {
                    magicSquare[face][i][j] = faceChars[face];
                }
            }
        }

        // 为了便于跟踪，在每个面的角上放置不同的字符
        magicSquare[0][0][0] = '0'; // back top-left
        magicSquare[1][0][0] = '1'; // down top-left
        magicSquare[2][0][0] = '2'; // front top-left
        magicSquare[3][0][0] = '3'; // left top-left
        magicSquare[4][0][0] = '4'; // right top-left
        magicSquare[5][0][0] = '5'; // up top-left
    }

    void print()
    {
        int order[4][3] = {
            {-1, 0, -1}, // 只显示 back
            {3, 5, 4},   // left, up, right
            {-1, 2, -1}, // 只显示 front
            {-1, 1, -1}  // 只显示 down
        };

        for (int i = 0; i < 4; i++)
        {
            for (int row = 0; row < SIZE; row++)
            {
                for (int j = 0; j < 3; j++)
                {
                    if (order[i][j] == -1)
                    {
                        printf("     ");
                    }
                    else
                    {
                        int faceIndex = order[i][j];
                        for (int col = 0; col < SIZE; col++)
                        {
                            printf("%c", magicSquare[faceIndex][row][col]);
                        }
                        printf(" ");
                    }
                }
                printf("\n");
            }
            if (i < 3)
                printf("\n");
        }
        printf("\n");
    }

    // 顺时针旋转 90°
    static Square rotateCW(const Square &a)
    {
        Square b{};
        for (int r = 0; r < SIZE; ++r)
            for (int c = 0; c < SIZE; ++c)
                b[c][SIZE - 1 - r] = a[r][c];
        return b;
    }

    // 逆时针旋转 90°
    static Square rotateCCW(const Square &a)
    {
        Square b{};
        for (int r = 0; r < SIZE; ++r)
            for (int c = 0; c < SIZE; ++c)
                b[SIZE - 1 - c][r] = a[r][c];
        return b;
    }

    void rotateFace(int index, int clockwise)
    {
        if (clockwise)
        {
            magicSquare[index] = rotateCW(magicSquare[index]);
        }
        else
        {
            magicSquare[index] = rotateCCW(magicSquare[index]);
        }
    }

    void rotate0521(int mun, bool clockwise) // 012
    {
        int order[] = {0, 5, 2, 1, 0};
        if (!clockwise)
        {
            order[0] = 0;
            order[1] = 1;
            order[2] = 2;
            order[3] = 5;
            order[4] = 0;
        }
        Line temp1, temp2;
        for (int i = 0; i < SIZE; i++)
        {
            temp1[i] = magicSquare[order[0]][i][mun];
        }
        for (int i = 1; i < 5; i++)
        {
            for (int j = 0; j < SIZE; j++)
            {
                temp2[j] = magicSquare[order[i]][j][mun];
            }
            for (int j = 0; j < SIZE; j++)
            {
                magicSquare[order[i]][j][mun] = temp1[j];
            }
            temp1 = temp2;
        }
        // 最后需要将最后一组数据写回到起始位置
        for (int j = 0; j < SIZE; j++)
        {
            magicSquare[order[0]][j][mun] = temp1[j];
        }
        if (mun == 0)
        {
            rotateFace(3, clockwise);
        }
        if (mun == 2)
        {
            rotateFace(4, !clockwise);
        }
    }

    void rotate(int mun, bool clockwise)
    {
        switch (mun)
        {
        case 0:
        case 1:
        case 2:
            rotate0521(mun, clockwise);
            break;
        }
    }
};

int main()
{
    TestMagicSquare ms;

    printf("Initial state:\n");
    ms.print();

    printf("After rotating column 0 clockwise:\n");
    ms.rotate(0, true);
    ms.print();

    printf("After rotating column 0 counter-clockwise (should return to initial):\n");
    ms.rotate(0, false);
    ms.print();

    return 0;
}
