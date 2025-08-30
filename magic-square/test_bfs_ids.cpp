#include <algorithm>
#include <iostream>
#include <vector>
#include <unordered_set>
#include <cstdio>
#include <array>
#include <stack>
#include <queue>
#include <chrono>
#include <set>

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

    // 创建一个简单的测试用例
    void createTestCase()
    {
        // 创建一个接近完成的魔方状态，只需要1-2步即可完成
        for (int i = 0; i < 6; i++)
        {
            for (int j = 0; j < SIZE; j++)
            {
                for (int k = 0; k < SIZE; k++)
                {
                    magicSquare[i][j][k] = '0' + i; // 每个面都是同一个数字
                }
            }
        }

        // 做一次旋转，让它不是完成状态
        rotate(0, true);
    }

    // 创建已完成的魔方
    void createSolvedCube()
    {
        for (int i = 0; i < 6; i++)
        {
            for (int j = 0; j < SIZE; j++)
            {
                for (int k = 0; k < SIZE; k++)
                {
                    magicSquare[i][j][k] = '0' + i;
                }
            }
        }
    }

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
    }

    Square rotateCW(const Square &a)
    {
        Square b{};
        for (int r = 0; r < SIZE; ++r)
            for (int c = 0; c < SIZE; ++c)
                b[c][SIZE - 1 - r] = a[r][c];
        return b;
    }

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

    void rotate0521(int mun, bool clockwise)
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

    void rotate2304(int mun, bool clockwise)
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

    void rotate1354(int mun, bool clockwise)
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
    printf("Solution found with %zu steps: ", path.size());
    for (auto node : path)
        printf("%d%c ", node.op, node.clockWise ? '+' : '-');
    printf("\n");
}

// 修正的 BFS 实现
bool bfs(MagicSquare head, int maxSteps = 10)
{
    nodes.clear();
    visited.clear();
    while (!q.empty())
        q.pop();

    nodes.push_back({head, -1, -1, false, 0});
    visited.insert(head.state());
    q.push(0);

    printf("Starting BFS...\n");
    int nodesExplored = 0;

    while (!q.empty())
    {
        int index = q.front();
        q.pop();
        Node current = nodes[index];
        nodesExplored++;

        if (nodesExplored % 1000 == 0)
        {
            printf("BFS: Explored %d nodes, current depth: %d, queue size: %zu\n",
                   nodesExplored, current.depth, q.size());
        }

        if (current.ms.check())
        {
            printf("BFS: Solution found!\n");
            printPath(index);
            return true;
        }

        if (current.depth >= maxSteps)
        {
            continue; // 超过最大步数，跳过扩展
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

    printf("BFS: No solution found within %d steps. Explored %d nodes.\n", maxSteps, nodesExplored);
    return false;
}

// 修正的 IDS 实现
bool ids(MagicSquare head, int maxDepth = 10)
{
    printf("Starting IDS...\n");

    for (int depth = 0; depth <= maxDepth; depth++)
    {
        printf("IDS: Trying depth %d\n", depth);

        nodes.clear();
        visited.clear();
        std::stack<int> st;

        nodes.push_back({head, -1, -1, false, 0});
        // IDS 的一个重要问题：每次迭代应该清除 visited 集合
        // 因为在不同深度限制下，可能需要重新访问某些状态
        st.push(0);

        int nodesExplored = 0;

        while (!st.empty())
        {
            int index = st.top();
            st.pop();
            Node current = nodes[index];
            nodesExplored++;

            if (current.ms.check())
            {
                printf("IDS: Solution found at depth %d!\n", depth);
                printPath(index);
                return true;
            }

            if (current.depth < depth) // 只有在深度小于当前限制时才扩展
            {
                for (int i = 8; i >= 0; --i)
                {
                    for (int j = 1; j >= 0; --j)
                    {
                        MagicSquare newMs = current.ms;
                        newMs.rotate(i, j == 1);
                        std::string state = newMs.state();

                        // IDS 问题：使用全局 visited 可能导致最优解丢失
                        // 在真正的 IDS 中，应该使用路径检测而不是全局状态检测
                        nodes.push_back({newMs, index, i, j == 1, current.depth + 1});
                        st.push(nodes.size() - 1);
                    }
                }
            }
        }

        printf("IDS: Depth %d completed, explored %d nodes\n", depth, nodesExplored);
    }

    printf("IDS: No solution found within depth %d\n", maxDepth);
    return false;
}

// 修正的 IDS 实现（无环检测版本）
bool ids_no_cycle(MagicSquare head, int maxDepth = 10)
{
    printf("Starting IDS (no cycle detection)...\n");

    for (int depth = 0; depth <= maxDepth; depth++)
    {
        printf("IDS-NC: Trying depth %d\n", depth);

        nodes.clear();
        std::stack<int> st;

        nodes.push_back({head, -1, -1, false, 0});
        st.push(0);

        int nodesExplored = 0;

        while (!st.empty())
        {
            int index = st.top();
            st.pop();
            Node current = nodes[index];
            nodesExplored++;

            if (current.ms.check())
            {
                printf("IDS-NC: Solution found at depth %d!\n", depth);
                printPath(index);
                return true;
            }

            if (current.depth < depth)
            {
                for (int i = 8; i >= 0; --i)
                {
                    for (int j = 1; j >= 0; --j)
                    {
                        MagicSquare newMs = current.ms;
                        newMs.rotate(i, j == 1);

                        // 检查是否回到了路径上的某个状态（避免循环）
                        std::string newState = newMs.state();
                        bool inPath = false;
                        for (int pathIdx = index; pathIdx != -1; pathIdx = nodes[pathIdx].father)
                        {
                            if (nodes[pathIdx].ms.state() == newState)
                            {
                                inPath = true;
                                break;
                            }
                        }

                        if (!inPath)
                        {
                            nodes.push_back({newMs, index, i, j == 1, current.depth + 1});
                            st.push(nodes.size() - 1);
                        }
                    }
                }
            }
        }

        printf("IDS-NC: Depth %d completed, explored %d nodes\n", depth, nodesExplored);
    }

    printf("IDS-NC: No solution found within depth %d\n", maxDepth);
    return false;
}

void testAlgorithms()
{
    printf("=== Testing BFS and IDS Algorithms ===\n\n");

    // 测试 1: 已完成的魔方（应该立即返回）
    printf("Test 1: Already solved cube\n");
    MagicSquare solved;
    solved.createSolvedCube();
    printf("Initial state (should be solved):\n");
    solved.print();
    printf("Is solved: %s\n", solved.check() ? "Yes" : "No");

    if (solved.check())
    {
        printf("✓ Cube is already solved, algorithms should return immediately\n");
    }
    printf("\n");

    // 测试 2: 简单的一步解决方案
    printf("Test 2: One-step solution\n");
    MagicSquare oneStep;
    oneStep.createSolvedCube();
    oneStep.rotate(0, true); // 做一次旋转
    printf("Initial state (one step from solved):\n");
    oneStep.print();
    printf("Is solved: %s\n", oneStep.check() ? "Yes" : "No");

    printf("\nTesting BFS on one-step case:\n");
    bool bfsResult = bfs(oneStep, 5);

    printf("\nTesting IDS on one-step case:\n");
    bool idsResult = ids(oneStep, 5);

    printf("\nTesting IDS-NC on one-step case:\n");
    bool idsNcResult = ids_no_cycle(oneStep, 5);

    printf("\n=== Results Summary ===\n");
    printf("BFS found solution: %s\n", bfsResult ? "Yes" : "No");
    printf("IDS found solution: %s\n", idsResult ? "Yes" : "No");
    printf("IDS-NC found solution: %s\n", idsNcResult ? "Yes" : "No");

    // 测试 3: 检查状态表示的一致性
    printf("\nTest 3: State representation consistency\n");
    MagicSquare test1, test2;
    test1.createSolvedCube();
    test2.createSolvedCube();

    printf("Two identical cubes have same state: %s\n",
           test1.state() == test2.state() ? "Yes" : "No");

    test1.rotate(0, true);
    test1.rotate(0, false); // 应该回到原状态

    printf("After rotate and counter-rotate, state matches: %s\n",
           test1.state() == test2.state() ? "Yes" : "No");
}

int main()
{
    reflaction['b'] = 0;
    reflaction['d'] = 1;
    reflaction['f'] = 2;
    reflaction['l'] = 3;
    reflaction['r'] = 4;
    reflaction['u'] = 5;

    testAlgorithms();

    return 0;
}
