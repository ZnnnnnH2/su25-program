#include <algorithm>
#include <iostream>
#include <vector>
#include <unordered_set>
#include <cstdio>
#include <array>
#include <stack>
#include <queue>
#include <unordered_map>
#include <limits>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <cerrno>
#include <cstring>
#include "./timer.cpp"
// #if __has_include(<filesystem>)
// #include <filesystem>
// namespace fs = std::filesystem;
// #elif __has_include(<experimental/filesystem>)
// #include <experimental/filesystem>
// namespace fs = std::experimental::filesystem;
// #else
// #error "Filesystem not supported by this compiler"
// #endif

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
        for (int i = 0; i < 6; ++i)
        {
            char c;
            while (true)
            {
                if (!std::cin.get(c))
                {
                    int e = errno; // 可能为0（流格式错误不一定设置 errno）
                    if (std::cin.eof())
                        std::cerr << "EOF while waiting face id at face index " << i << "\n";
                    else if (std::cin.fail())
                        std::cerr << "Stream fail while reading face id (i=" << i << ")\n";
                    if (e)
                        std::cerr << "errno=" << e << " (" << std::strerror(e) << ")\n";
                    return;
                }
                if (c == 'b' || c == 'd' || c == 'f' || c == 'l' || c == 'r' || c == 'u')
                    break;
            }
            int index = reflaction[c];
            std::string dummy;
            if (!(std::cin >> dummy))
            {
                int e = errno;
                std::cerr << "Failed to read face header token after face id: " << c << " (face sequence=" << i << ")\n";
                if (std::cin.eof())
                    std::cerr << "Reason: EOF encountered.\n";
                else if (std::cin.bad())
                    std::cerr << "Reason: stream bad (I/O error).\n";
                else if (std::cin.fail())
                    std::cerr << "Reason: format fail.\n";
                if (e)
                    std::cerr << "errno=" << e << " (" << std::strerror(e) << ")\n";
                return;
            }
            for (int r = 0; r < SIZE; ++r)
            {
                for (int col = 0; col < SIZE; ++col)
                {
                    if (!(std::cin >> magicSquare[index][r][col]))
                    {
                        int e = errno;
                        std::cerr << "Failed to read cell for face '" << c << "' at (" << r << "," << col << ") faceSeq=" << i << "\n";
                        if (std::cin.eof())
                            std::cerr << "Reason: EOF encountered.\n";
                        else if (std::cin.bad())
                            std::cerr << "Reason: stream bad (I/O error).\n";
                        else if (std::cin.fail())
                            std::cerr << "Reason: format fail.\n";
                        if (e)
                            std::cerr << "errno=" << e << " (" << std::strerror(e) << ")\n";
                        return;
                    }
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

        for (int i = 0; i < 4; ++i)
        {
            for (int row = 0; row < SIZE; ++row)
            {
                for (int j = 0; j < 3; ++j)
                {
                    if (order[i][j] == -1)
                    {
                        std::cout << "     ";
                    }
                    else
                    {
                        int faceIndex = order[i][j];
                        for (int col = 0; col < SIZE; ++col)
                        {
                            std::cout << magicSquare[faceIndex][row][col];
                        }
                        std::cout << ' ';
                    }
                }
                std::cout << '\n';
            }
            if (i < 3)
                std::cout << '\n';
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
            if (i == 4)
                continue;
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
    bool isSolved()
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
    MagicSquare cube;  // 立方体状态
    std::string state; // 缓存字符串状态，避免重复生成
    int g_cost;        // 已花费代价
    int f_cost;        // g + h
    int parent;        // 父节点索引（-1 表示根）
    uint8_t move;      // 执行的操作编号 0..8（根节点可设 255）
    uint8_t dir;       // 方向：0 = 逆时针，1 = 顺时针
};

struct PQItem
{
    int f_cost;
    int g_cost;
    int index; // nodes 向量中的索引
    bool operator<(const PQItem &other) const
    {
        // priority_queue 为大顶堆，这里反转使最小 f_cost 优先
        if (f_cost != other.f_cost)
            return f_cost > other.f_cost;
        return g_cost > other.g_cost; // 次级依据：更小 g
    }
};

std::priority_queue<PQItem> pq;
std::unordered_map<std::string, int> gScore; // 记忆最优 g 值
std::vector<Node> nodes;                     // 存放所有节点，便于父指针回溯

int heuristic(const MagicSquare &ms)
{
    // 目前返回 0，未来可直接替换为更强启发式，不影响其它结构
    return 0;
}

std::vector<std::pair<int, int>> reconstructPath(int goalIndex)
{
    std::vector<std::pair<int, int>> path;
    for (int i = goalIndex; i != -1; i = nodes[i].parent)
    {
        if (nodes[i].parent == -1)
            break; // 根节点无操作
        path.push_back({nodes[i].move, nodes[i].dir});
    }
    std::reverse(path.begin(), path.end());
    return path;
}

std::vector<std::pair<int, int>> AStar(MagicSquare &start)
{
    // 清理数据结构
    gScore.clear();
    nodes.clear();
    while (!pq.empty())
        pq.pop();
    gScore.reserve(200000); // 预留，减少 rehash

    // 初始化根节点
    std::string s = start.state();
    nodes.push_back({start, s, 0, heuristic(start), -1, 255, 0});
    gScore[s] = 0;
    pq.push({nodes[0].f_cost, 0, 0});

    while (!pq.empty())
    {
        PQItem item = pq.top();
        pq.pop();
        int idx = item.index;
        Node &cur = nodes[idx];

        // 过期节点（有更优 g 已记录）
        if (cur.g_cost > gScore[cur.state])
            continue;

        // 目标检测：首次弹出即最优（单一代价 + 一致启发）
        if (cur.cube.isSolved())
        {
            return reconstructPath(idx);
        }

        // 扩展
        for (int mv = 0; mv < 9; ++mv)
        {
            for (int d = 0; d < 2; ++d)
            {
                // 简单剪枝：避免直接反向 (上一步与本步为同一 mv 且方向相反)
                if (cur.parent != -1 && cur.move == mv && cur.dir != d)
                    continue;

                MagicSquare next = cur.cube;
                next.rotate(mv, d == 1);
                std::string ns = next.state();
                int tentative_g = cur.g_cost + 1;
                auto it = gScore.find(ns);
                if (it != gScore.end() && tentative_g >= it->second)
                    continue; // 不是更优

                int h = heuristic(next);
                int f = tentative_g + h;
                int newIndex;
                if (it == gScore.end())
                {
                    gScore.emplace(ns, tentative_g);
                }
                else
                {
                    it->second = tentative_g;
                }
                nodes.push_back({next, ns, tentative_g, f, idx, static_cast<uint8_t>(mv), static_cast<uint8_t>(d)});
                newIndex = (int)nodes.size() - 1;
                pq.push({f, tentative_g, newIndex});
            }
        }
    }
    // 未找到解，返回空
    return {};
}
int main()
{
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);
    std::cout << "no input file found";
    // std::cout << fs::current_path() << std::endl;
    std::ifstream fin("4.in");
    if (fin)
    {
        std::cin.rdbuf(fin.rdbuf());
    }
    else
    {
        // std::cout << std::filesystem::current_path() << std::endl;
        int e = errno; // 打开文件失败时的 errno
        std::cerr << "Warning: could not open 4.in. Fallback to stdin.\n";
        if (e)
            std::cerr << "open errno=" << e << " (" << std::strerror(e) << ")\n";
    }

    reflaction['b'] = 0;
    reflaction['d'] = 1;
    reflaction['f'] = 2;
    reflaction['l'] = 3;
    reflaction['r'] = 4;
    reflaction['u'] = 5;
    MagicSquare ms;
    ms.readIn();
    ms.print();

    Timer timer;
    timer.reset();
    auto path = AStar(ms);
    double t = timer.stop();
    for (auto &op : path)
        std::cout << op.first << (op.second ? '+' : '-');
    std::cout << '\n';
    std::cout << "Steps: " << path.size() << '\n';
    std::cout << std::fixed << std::setprecision(6)
              << "Time consumption: " << t << " seconds\n";
    return 0;
}