#include <iostream>
#include <array>

#define SIZE 3

typedef std::array<std::array<char, SIZE>, SIZE> Square;
typedef std::array<char, SIZE> Line;

class TestCube
{
private:
    Square magicSquare[6]; // 0: back, 1: down, 2: front, 3: left, 4: right, 5: up

public:
    void init()
    {
        // 初始化一个简单的测试立方体
        for (int face = 0; face < 6; face++)
        {
            char baseChar = 'A' + face;
            for (int i = 0; i < SIZE; i++)
            {
                for (int j = 0; j < SIZE; j++)
                {
                    magicSquare[face][i][j] = baseChar;
                }
            }
        }

        // 为每个面的不同位置设置不同的字符以便跟踪
        for (int face = 0; face < 6; face++)
        {
            for (int i = 0; i < SIZE; i++)
            {
                for (int j = 0; j < SIZE; j++)
                {
                    magicSquare[face][i][j] = '0' + face * 10 + i * 3 + j;
                }
            }
        }
    }

    void printFace(int faceIndex)
    {
        std::cout << "Face " << faceIndex << ":" << std::endl;
        for (int i = 0; i < SIZE; i++)
        {
            for (int j = 0; j < SIZE; j++)
            {
                std::cout << magicSquare[faceIndex][i][j] << " ";
            }
            std::cout << std::endl;
        }
        std::cout << std::endl;
    }

    void printColumn(int col)
    {
        std::cout << "Column " << col << " rotation path:" << std::endl;
        std::cout << "Back[*][" << col << "]: ";
        for (int i = 0; i < SIZE; i++)
        {
            std::cout << magicSquare[0][i][col] << " ";
        }
        std::cout << std::endl;

        std::cout << "Up[*][" << col << "]: ";
        for (int i = 0; i < SIZE; i++)
        {
            std::cout << magicSquare[5][i][col] << " ";
        }
        std::cout << std::endl;

        std::cout << "Front[*][" << col << "]: ";
        for (int i = 0; i < SIZE; i++)
        {
            std::cout << magicSquare[2][i][col] << " ";
        }
        std::cout << std::endl;

        std::cout << "Down[*][" << col << "]: ";
        for (int i = 0; i < SIZE; i++)
        {
            std::cout << magicSquare[1][i][col] << " ";
        }
        std::cout << std::endl
                  << std::endl;
    }
};

int main()
{
    TestCube cube;
    cube.init();

    std::cout << "Initial state:" << std::endl;
    for (int i = 0; i < 6; i++)
    {
        cube.printFace(i);
    }

    // 测试列旋转
    cube.printColumn(0);
    cube.printColumn(1);
    cube.printColumn(2);

    return 0;
}
