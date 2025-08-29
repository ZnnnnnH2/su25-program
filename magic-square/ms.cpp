#include <algorithm>
#include <iostream>
#include <vector>
#include <unordered_set>
#include <cstdio>
#include <array>
#include <stack>
#include <queue>

#define SIZE 3

typedef std::array<std::array<char, SIZE>, SIZE> Square;
typedef std::array<char, SIZE> Line;

int reflaction[200];

class MagicSquare
{
private:
    Square magicSquare[6]; // 0: back, 1: down, 2: front, 3: left, 4: right, 5: up

public:
    MagicSquare() {}
    void readIn()
    {
        for (int i = 0; i < 6; i++)
        {
            char c;
            do
            {
                int t = scanf("%c", &c);
                if (t == -1)
                {
                    printf("EOF encountered, t = %d\n", t);
                    return;
                }
            } while (c != 'b' && c != 'd' && c != 'f' && c != 'l' && c != 'r' && c != 'u');
            // putchar(c);
            // putchar('\n');
            int index = reflaction[c];
            scanf("%*s");
            for (int j = 0; j < SIZE; j++)
            {
                for (int k = 0; k < SIZE; k++)
                {
                    scanf(" %c", &magicSquare[index][j][k]);
                }
            }
        }
    }
    void print()
    {
        int order[4][3] = {
            {-1, 0, -1},
            {3, 5, 4},
            {-1, 2, -1},
            {-1, 1, -1}};

        // Print the magic square layout according to order array
        for (int i = 0; i < 4; i++)
        {
            for (int row = 0; row < SIZE; row++)
            {
                for (int j = 0; j < 3; j++)
                {
                    if (order[i][j] == -1)
                    {
                        // Print empty space
                        printf("     ");
                    }
                    else
                    {
                        // Print the face
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
                printf("\n"); // Add spacing between rows
        }
    }
    // 顺时针旋转 90°
    Square rotateCW(const Square &a)
    {
        Square b{};
        for (int r = 0; r < SIZE; ++r)
            for (int c = 0; c < SIZE; ++c)
                b[c][SIZE - 1 - r] = a[r][c];
        return b;
    }

    // 逆时针旋转 90°
    Square rotateCCW(const Square &a)
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
        // std::reverse(temp1.begin(), temp1.end());
        for (int i = 0; i < SIZE; i++)
        {
            magicSquare[order[0]][i][mun] = temp1[i];
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
    void rotate2304(int mun, bool clockwise) // 345
    {
        int order[] = {2, 3, 0, 4, 2};
        if (!clockwise)
        {
            order[0] = 2;
            order[1] = 4;
            order[2] = 0;
            order[3] = 3;
            order[4] = 2;
        }
        int munForAll[5];
        munForAll[0] = mun - SIZE;
        munForAll[2] = 2 * SIZE - mun - 1;
        munForAll[3] = mun - SIZE;
        munForAll[4] = 2 * SIZE - mun - 1;
        Line temp1, temp2;
        for (int i = 0; i < SIZE; i++)
        {
            temp1[i] = magicSquare[order[0]][munForAll[order[0]]][i];
        }
        if (!clockwise)
        {
            std::reverse(temp1.begin(), temp1.end());
        }
        for (int i = 1; i < 5; i++)
        {
            for (int j = 0; j < SIZE; j++)
            {
                if (order[i] == 2 || order[i] == 0)
                {
                    temp2[j] = magicSquare[order[i]][munForAll[order[i]]][j];
                }
                else
                {
                    temp2[j] = magicSquare[order[i]][j][munForAll[order[i]]];
                }
            }
            for (int j = 0; j < SIZE; j++)
            {
                if (order[i] == 2 || order[i] == 0)
                {
                    magicSquare[order[i]][munForAll[order[i]]][j] = temp1[j];
                }
                else
                {
                    magicSquare[order[i]][j][munForAll[order[i]]] = temp1[j];
                }
            }
            if (order[i] == 3 and order[i + 1] == 2 || order[i] == 2 and order[i + 1] == 3 || order[i] == 4 and order[i + 1] == 0 || order[i] == 0 and order[i + 1] == 4)
            {
                temp1 = temp2;
            }
            else
            {
                std::reverse(temp2.begin(), temp2.end());
                temp1 = temp2;
            }
        }
        if (mun == 3)
        {
            rotateFace(1, !clockwise);
        }
        if (mun == 5)
        {
            rotateFace(5, clockwise);
        }
    }
    void rotate1354(int mun, bool clockwise) // 678
    {
        int order[] = {1, 3, 5, 4, 1};
        if (!clockwise)
        {
            order[0] = 1;
            order[1] = 4;
            order[2] = 5;
            order[3] = 3;
            order[4] = 1;
        }
        int munForDown = mun - 2 * SIZE;
        int munForUp = 3 * SIZE - mun - 1;
        int munForAll[] = {munForDown, munForUp, munForUp, munForUp, munForDown};
        Line temp1, temp2;
        for (int i = 0; i < SIZE; i++)
        {
            temp1[i] = magicSquare[order[0]][munForAll[0]][i];
        }
        std::reverse(temp1.begin(), temp1.end());
        for (int i = 1; i < 4; i++)
        {
            for (int j = 0; j < SIZE; j++)
            {
                temp2[j] = magicSquare[order[i]][munForAll[i]][j];
            }
            for (int j = 0; j < SIZE; j++)
            {
                magicSquare[order[i]][munForAll[i]][j] = temp1[j];
            }
            temp1 = temp2;
        }
        std::reverse(temp1.begin(), temp1.end());
        for (int i = 0; i < SIZE; i++)
        {
            magicSquare[order[0]][munForAll[0]][i] = temp1[i];
        }
        if (mun == 6)
        {
            rotateFace(2, clockwise);
        }
        if (mun == 8)
        {
            rotateFace(0, !clockwise);
        }
    }
    void rotate(int mun, bool clockwise) // 1 for + 0 for -
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
    bool check()
    {
        for (int i = 0; i < 6; i++)
        {
            for (int j = 0; j < SIZE; j++)
            {
                for (int k = 0; k < SIZE; k++)
                {
                    if (magicSquare[i][0][0] != magicSquare[i][j][k])
                    {
                        return false;
                    }
                }
            }
        }
        return true;
    }
    std::string state()
    {
        std::string str;

        for (int f = 0; f < 6; f++)
        {
            for (int r = 0; r < SIZE; r++)
            {
                for (int c = 0; c < SIZE; c++)
                {
                    str += magicSquare[f][r][c];
                }
            }
        }
        return str;
    }
};

struct Node
{
    MagicSquare ms;
    int father;
    int op;
    bool clockWise;
    int depth;
};

std::vector<Node> nodes;
std::queue<int> q;
std::unordered_set<std::string> visited;
void printPath(int goalIdx)
{
    std::vector<Node> path;
    for (int i = goalIdx; i != -1; i = nodes[i].father)
    {
        if (nodes[i].father != -1)
            path.push_back(nodes[i]);
    }
    std::reverse(path.begin(), path.end());
    for (auto node : path)
        printf("%d%c ", node.op, node.clockWise ? '+' : '-');
}
void bfs(MagicSquare head)
{
    nodes.clear();
    visited.clear();
    while (!q.empty())
        q.pop();
    nodes.push_back({head, -1, -1, false, 0});
    visited.insert(head.state());
    q.push(0);
    while (!q.empty())
    {
        int index = q.front();
        // printf("Visiting node %d\n", index);
        q.pop();
        Node current = nodes[index];
        if (current.ms.check())
        {
            printPath(index);
            return;
        }
        for (int i = 0; i < 9; i++)
        {
            for (int j = 0; j < 2; j++)
            {
                MagicSquare newMs = current.ms;
                newMs.rotate(i, j == 1);
                std::string state = newMs.state();
                if (visited.find(state) == visited.end())
                {
                    visited.insert(state);
                    nodes.push_back({newMs, index, i, j == 1, current.depth + 1});
                    q.push(nodes.size() - 1);
                }
            }
        }
    }
}

void ids(MagicSquare head)
{
    for (int maxDepth = 0; maxDepth <= 20; maxDepth++) // 限制最大深度避免无限搜索
    {
        nodes.clear();
        visited.clear();
        std::stack<int> st;
        nodes.push_back({head, -1, -1, false, 0});
        visited.insert(head.state());
        st.push(0);

        while (!st.empty())
        {
            int index = st.top();
            st.pop();
            Node current = nodes[index];

            if (current.ms.check())
            {
                printPath(index);
                return;
            }

            if (current.depth < maxDepth) // 只有在深度小于限制时才扩展
            {
                for (int i = 8; i >= 0; --i)
                {
                    for (int j = 1; j >= 0; --j)
                    {
                        MagicSquare newMs = current.ms;
                        newMs.rotate(i, j == 1);
                        std::string state = newMs.state();
                        if (visited.find(state) == visited.end())
                        {
                            visited.insert(state);
                            nodes.push_back({newMs, index, i, j == 1, current.depth + 1});
                            st.push(nodes.size() - 1);
                        }
                    }
                }
            }
        }
    }
}

// 简单的深度优先搜索（不限制深度）
void dfs(MagicSquare head)
{
    nodes.clear();
    visited.clear();
    std::stack<int> st;
    nodes.push_back({head, -1, -1, false, 0});
    visited.insert(head.state());
    st.push(0);

    while (!st.empty())
    {
        int index = st.top();
        st.pop();
        Node current = nodes[index];

        if (current.ms.check())
        {
            printPath(index);
            return;
        }

        // 按相反顺序压栈，使得较小编号的操作先被探索
        for (int i = 8; i >= 0; --i)
        {
            for (int j = 1; j >= 0; --j)
            {
                MagicSquare newMs = current.ms;
                newMs.rotate(i, j == 1);
                std::string state = newMs.state();
                if (visited.find(state) == visited.end())
                {
                    visited.insert(state);
                    nodes.push_back({newMs, index, i, j == 1, current.depth + 1});
                    st.push(nodes.size() - 1);
                }
            }
        }
    }
}

int main()
{
    freopen("4.in", "r", stdin);
    freopen("4.out", "w", stdout);

    reflaction['b'] = 0;
    reflaction['d'] = 1;
    reflaction['f'] = 2;
    reflaction['l'] = 3;
    reflaction['r'] = 4;
    reflaction['u'] = 5;
    MagicSquare ms;
    ms.readIn();
    ms.print();
    // 3- 6+ 4- 7+ 1-
    // ms.rotate(3, false);
    // ms.rotate(6, true);
    // ms.rotate(4, false);
    // ms.rotate(7, true);
    // ms.rotate(1, false);
    // ms.print();
    // bfs(ms);
    dfs(ms);
    // ids(ms);
    return 0;
}