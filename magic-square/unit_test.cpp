#include <algorithm>
#include <iostream>
#include <vector>
#include <unordered_set>
#include <cstdio>
#include <array>
#include <cassert>

#define SIZE 3

typedef std::array<std::array<char, SIZE>, SIZE> Square;
typedef std::array<char, SIZE> Line;

class MagicSquareTest
{
private:
    Square magicSquare[6]; // 0: back, 1: down, 2: front, 3: left, 4: right, 5: up

public:
    MagicSquareTest() {}

    void initTest()
    {
        // 初始化一个标准的测试魔方，每个面有唯一标识
        char faceNames[] = {'B', 'D', 'F', 'L', 'R', 'U'};
        for (int face = 0; face < 6; face++)
        {
            for (int i = 0; i < SIZE; i++)
            {
                for (int j = 0; j < SIZE; j++)
                {
                    // 格式: 面名 + 行号 + 列号
                    magicSquare[face][i][j] = '0' + face * 10 + i * 3 + j;
                }
            }
        }
    }

    void print()
    {
        int order[4][3] = {
            {-1, 0, -1}, // back
            {3, 5, 4},   // left, up, right
            {-1, 2, -1}, // front
            {-1, 1, -1}  // down
        };

        for (int i = 0; i < 4; i++)
        {
            for (int row = 0; row < SIZE; row++)
            {
                for (int j = 0; j < 3; j++)
                {
                    if (order[i][j] == -1)
                    {
                        printf("       ");
                    }
                    else
                    {
                        int faceIndex = order[i][j];
                        for (int col = 0; col < SIZE; col++)
                        {
                            printf("%c ", magicSquare[faceIndex][row][col]);
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

    void printCompact()
    {
        printf("Back: ");
        for (int i = 0; i < SIZE; i++)
            for (int j = 0; j < SIZE; j++)
                printf("%c", magicSquare[0][i][j]);
        printf(" Down: ");
        for (int i = 0; i < SIZE; i++)
            for (int j = 0; j < SIZE; j++)
                printf("%c", magicSquare[1][i][j]);
        printf(" Front: ");
        for (int i = 0; i < SIZE; i++)
            for (int j = 0; j < SIZE; j++)
                printf("%c", magicSquare[2][i][j]);
        printf(" Left: ");
        for (int i = 0; i < SIZE; i++)
            for (int j = 0; j < SIZE; j++)
                printf("%c", magicSquare[3][i][j]);
        printf(" Right: ");
        for (int i = 0; i < SIZE; i++)
            for (int j = 0; j < SIZE; j++)
                printf("%c", magicSquare[4][i][j]);
        printf(" Up: ");
        for (int i = 0; i < SIZE; i++)
            for (int j = 0; j < SIZE; j++)
                printf("%c", magicSquare[5][i][j]);
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

    void rotateFace(int index, bool clockwise)
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

    // 列旋转 (0,1,2) - 类似 L, M, R
    void rotate0521(int mun, bool clockwise)
    {
        printf("Rotating column %d %s\n", mun, clockwise ? "clockwise" : "counter-clockwise");

        // 顺时针：back -> up -> front -> down -> back
        int order[] = {0, 5, 2, 1, 0};
        if (!clockwise)
        {
            // 逆时针：back -> down -> front -> up -> back
            order[1] = 1;
            order[2] = 2;
            order[3] = 5;
        }

        Line temp1, temp2;
        for (int i = 0; i < SIZE; i++)
        {
            temp1[i] = magicSquare[order[0]][i][mun];
        }

        for (int i = 1; i < 4; i++)
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

        // 将最后保存的数据写回起始位置
        for (int j = 0; j < SIZE; j++)
        {
            magicSquare[order[0]][j][mun] = temp1[j];
        }

        // 旋转相关面
        if (mun == 0)
        {
            rotateFace(3, !clockwise); // left面
        }
        if (mun == 2)
        {
            rotateFace(4, clockwise); // right面
        }
    }

    // 行旋转 (3,4,5) - 类似 D, E, U
    void rotate2304(int mun, bool clockwise)
    {
        printf("Rotating row %d %s\n", mun - 3, clockwise ? "clockwise" : "counter-clockwise");

        // 顺时针：front -> left -> back -> right -> front
        int order[] = {2, 3, 0, 4, 2};
        if (!clockwise)
        {
            // 逆时针：front -> right -> back -> left -> front
            order[1] = 4;
            order[3] = 3;
        }

        int munForAll[5];
        int row = mun - SIZE;
        munForAll[0] = row;            // front
        munForAll[1] = row;            // left
        munForAll[2] = SIZE - 1 - row; // back (反向)
        munForAll[3] = row;            // right
        munForAll[4] = row;            // front

        Line temp1, temp2;
        for (int i = 0; i < SIZE; i++)
        {
            temp1[i] = magicSquare[order[0]][munForAll[0]][i];
        }

        for (int i = 1; i < 4; i++)
        {
            for (int j = 0; j < SIZE; j++)
            {
                if (order[i] == 0)
                { // back面，需要反向
                    temp2[j] = magicSquare[order[i]][munForAll[i]][SIZE - 1 - j];
                }
                else
                {
                    temp2[j] = magicSquare[order[i]][munForAll[i]][j];
                }
            }
            for (int j = 0; j < SIZE; j++)
            {
                if (order[i] == 0)
                { // back面，需要反向
                    magicSquare[order[i]][munForAll[i]][SIZE - 1 - j] = temp1[j];
                }
                else
                {
                    magicSquare[order[i]][munForAll[i]][j] = temp1[j];
                }
            }
            temp1 = temp2;
        }

        // 将最后保存的数据写回起始位置
        for (int j = 0; j < SIZE; j++)
        {
            magicSquare[order[0]][munForAll[0]][j] = temp1[j];
        }

        // 旋转相关面
        if (mun == 3)
        {
            rotateFace(1, clockwise); // down面
        }
        if (mun == 5)
        {
            rotateFace(5, !clockwise); // up面
        }
    }

    // 层旋转 (6,7,8) - 类似 B, S, F
    void rotate1354(int mun, bool clockwise)
    {
        printf("Rotating layer %d %s\n", mun - 6, clockwise ? "clockwise" : "counter-clockwise");

        // 顺时针：up -> right -> down -> left -> up
        int order[] = {5, 4, 1, 3, 5};
        if (!clockwise)
        {
            // 逆时针：up -> left -> down -> right -> up
            order[1] = 3;
            order[3] = 4;
        }

        int row = mun - 2 * SIZE;

        Line temp1, temp2;
        for (int i = 0; i < SIZE; i++)
        {
            temp1[i] = magicSquare[order[0]][row][i];
        }

        for (int i = 1; i < 4; i++)
        {
            for (int j = 0; j < SIZE; j++)
            {
                temp2[j] = magicSquare[order[i]][row][j];
            }
            for (int j = 0; j < SIZE; j++)
            {
                magicSquare[order[i]][row][j] = temp1[j];
            }
            temp1 = temp2;
        }

        // 将最后保存的数据写回起始位置
        for (int j = 0; j < SIZE; j++)
        {
            magicSquare[order[0]][row][j] = temp1[j];
        }

        // 旋转相关面
        if (mun == 6)
        {
            rotateFace(2, clockwise); // front面
        }
        if (mun == 8)
        {
            rotateFace(0, !clockwise); // back面
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
        case 3:
        case 4:
        case 5:
            rotate2304(mun, clockwise);
            break;
        case 6:
        case 7:
        case 8:
            rotate1354(mun, clockwise);
            break;
        }
    }

    // 获取当前状态的字符串表示
    std::string getState()
    {
        std::string state;
        for (int face = 0; face < 6; face++)
        {
            for (int i = 0; i < SIZE; i++)
            {
                for (int j = 0; j < SIZE; j++)
                {
                    state += magicSquare[face][i][j];
                }
            }
        }
        return state;
    }

    // 测试旋转的可逆性
    bool testReversibility(int mun)
    {
        printf("\n=== Testing reversibility for rotation %d ===\n", mun);
        std::string initial = getState();
        printf("Initial state: ");
        printCompact();

        // 顺时针旋转
        rotate(mun, true);
        printf("After clockwise rotation: ");
        printCompact();

        // 逆时针旋转
        rotate(mun, false);
        printf("After counter-clockwise rotation: ");
        printCompact();

        std::string final = getState();
        bool success = (initial == final);
        printf("Reversibility test %s\n", success ? "PASSED" : "FAILED");

        return success;
    }

    // 测试四次旋转回到原状态
    bool testFourRotations(int mun)
    {
        printf("\n=== Testing four rotations for rotation %d ===\n", mun);
        std::string initial = getState();
        printf("Initial state: ");
        printCompact();

        for (int i = 1; i <= 4; i++)
        {
            rotate(mun, true);
            printf("After rotation %d: ", i);
            printCompact();
        }

        std::string final = getState();
        bool success = (initial == final);
        printf("Four rotations test %s\n", success ? "PASSED" : "FAILED");

        return success;
    }
};

int main()
{
    printf("Magic Cube Rotation Unit Test\n");
    printf("=============================\n\n");

    MagicSquareTest cube;
    cube.initTest();

    printf("Initial cube state:\n");
    cube.print();

    int passed = 0;
    int total = 0;

    // 测试所有9个旋转操作的可逆性
    for (int i = 0; i < 9; i++)
    {
        cube.initTest(); // 重新初始化
        total++;
        if (cube.testReversibility(i))
        {
            passed++;
        }
    }

    // 测试所有9个旋转操作的四次旋转
    for (int i = 0; i < 9; i++)
    {
        cube.initTest(); // 重新初始化
        total++;
        if (cube.testFourRotations(i))
        {
            passed++;
        }
    }

    printf("\n=============================\n");
    printf("Test Results: %d/%d tests passed\n", passed, total);
    printf("Success rate: %.1f%%\n", (float)passed / total * 100);

    if (passed == total)
    {
        printf("All tests PASSED! ✓\n");
    }
    else
    {
        printf("Some tests FAILED! ✗\n");
    }

    return 0;
}
