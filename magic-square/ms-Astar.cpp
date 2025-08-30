#include <algorithm>
#include <iostream>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <cstdio>
#include <array>
#include <stack>
#include <queue>
#include "./timer.cpp"

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
            if (order[i] == 3 && order[i + 1] == 2 || order[i] == 2 && order[i + 1] == 3 || order[i] == 4 && order[i + 1] == 0 || order[i] == 0 && order[i + 1] == 4)
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

    // 添加方法用于启发式函数访问
    char getCell(int face, int row, int col) const
    {
        return magicSquare[face][row][col];
    }
};

int heuristic(const MagicSquare &ms)
{
    // 示例：计算不匹配面的数量作为启发式
    // 这是可接受的，因为每步最多修复1个面，所以不会高估
    int mismatched_faces = 0;
    for (int i = 0; i < 6; i++)
    {
        char first_color = ms.getCell(i, 0, 0);
        for (int j = 0; j < SIZE; j++)
        {
            for (int k = 0; k < SIZE; k++)
            {
                if (ms.getCell(i, j, k) != first_color)
                {
                    mismatched_faces++;
                    goto next_face; // 这个面有不匹配，计数并跳到下一面
                }
            }
        }
    next_face:;
    }
    return mismatched_faces;

    // 或者简单返回0保持原来的行为
    // return 0;
}

struct Node
{
    MagicSquare cube;
    std::string state;
    int g_cost;
    int f_cost;
    int parent;
    int move;
    int dir;
};

struct PQItem
{
    int f_cost;
    int g_cost;
    int index;
    bool operator<(const PQItem &other) const
    {
        if (f_cost != other.f_cost)
            return f_cost > other.f_cost;
        return g_cost > other.g_cost;
    }
};

std::vector<std::pair<int, int>> reconstructPath(const std::vector<Node> &nodes, int goalIndex)
{
    std::vector<std::pair<int, int>> path;
    for (int i = goalIndex; i != -1; i = nodes[i].parent)
    {
        if (nodes[i].parent == -1)
            break;
        path.push_back({nodes[i].move, nodes[i].dir});
    }
    std::reverse(path.begin(), path.end());
    return path;
}
void AStar(MagicSquare ms)
{
    std::priority_queue<PQItem> pq;
    std::unordered_map<std::string, int> gScore;
    std::vector<Node> nodes;

    // 清理全局状态（如果有的话）
    while (!pq.empty())
        pq.pop();
    gScore.clear();
    nodes.clear();

    // 初始化根节点
    std::string s = ms.state();
    nodes.push_back({ms, s, 0, heuristic(ms), -1, -1, 0});
    gScore[s] = 0;
    pq.push({nodes[0].f_cost, 0, 0});

    int expanded = 0;

    while (!pq.empty())
    {
        PQItem item = pq.top();
        pq.pop();
        int idx = item.index;

        if (idx < 0 || idx >= (int)nodes.size())
            continue;

        // 使用值拷贝而不是引用，避免vector重新分配导致的引用失效
        Node cur = nodes[idx];

        // 过期节点检查
        if (cur.g_cost > gScore[cur.state])
            continue;

        expanded++;

        // 目标检测
        if (cur.cube.check())
        {
            auto path = reconstructPath(nodes, idx);
            printf("Found solution with %d steps!\n", (int)path.size());
            printf("Expanded %d nodes\n", expanded);
            printf("Solution: ");
            for (auto &op : path)
            {
                printf("%d%c ", op.first, op.second ? '+' : '-');
            }
            printf("\n");
            return;
        }

        // 扩展邻居
        for (int mv = 0; mv < 9; ++mv)
        {
            for (int d = 0; d < 2; ++d)
            {
                // 简单剪枝：避免直接反向操作
                if (cur.parent != -1 && cur.move == mv && cur.dir != d)
                    continue;

                MagicSquare next = cur.cube;
                next.rotate(mv, d == 1);
                std::string ns = next.state();
                int tentative_g = cur.g_cost + 1;

                auto it = gScore.find(ns);
                if (it != gScore.end() && tentative_g >= it->second)
                    continue;

                int h = heuristic(next);
                int f = tentative_g + h;

                if (it == gScore.end())
                {
                    gScore.emplace(ns, tentative_g);
                }
                else
                {
                    it->second = tentative_g;
                }

                nodes.push_back({next, ns, tentative_g, f, idx, mv, d});
                int newIndex = (int)nodes.size() - 1;
                pq.push({f, tentative_g, newIndex});
            }
        }

        // 添加进度输出（每1000个节点输出一次）
        if (expanded % 1000 == 0)
        {
            printf("Expanded %d nodes, queue size: %d\n", expanded, (int)pq.size());
        }
    }

    printf("No solution found after expanding %d nodes\n", expanded);
}

int main()
{
    freopen("1.in", "r", stdin);
    // freopen("4.out", "w", stdout);

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
    Timer timer;
    timer.reset();
    // bfs(ms);
    // dfs(ms);
    AStar(ms);
    double T = timer.stop();
    printf("Time consumption: %.6f seconds\n", T);

    return 0;
}