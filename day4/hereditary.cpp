#include <iostream>
#include <vector>
#include <cstdlib>
#include <queue>
#include <algorithm>
#include <ctime>

#define SIZE 1028 // 为了便于演示和快速找到解，将棋盘大小减小到8

#define N 100             // 种群大小
#define ABERRANCERATE 0.2 // 变异率

struct Board
{
    std::vector<int> queens; // queens[i] 表示第i行皇后所在的列
    int score;               // 冲突数

    Board() : queens(SIZE), score(0) {}
    Board(std::vector<int> q, int s) : queens(q), score(s) {}

    // 为了在priority_queue中实现最小堆（分数越低越优先）
    bool operator>(const Board &other) const
    {
        return score > other.score;
    }
};

// 优先队列，用于存储种群，分数（冲突数）最低的在顶部
std::priority_queue<Board, std::vector<Board>, std::greater<Board>> boardQueue;

// 计算在给定棋盘上，位于(row, col)的皇后的冲突数
int getConflicts(const std::vector<int> &board_queens, int row, int col)
{
    int cnt = 0;
    for (int i = 0; i < SIZE; ++i)
    {
        if (i == row)
            continue;
        // 检查列冲突
        if (board_queens[i] == col)
            cnt++;
        // 检查对角线冲突
        if (abs(i - row) == abs(board_queens[i] - col))
            cnt++;
    }
    return cnt;
}

// 统计整个棋盘的总冲突数
int totalConflicts(const std::vector<int> &board_queens)
{
    int total = 0;
    for (int row = 0; row < SIZE; ++row)
    {
        total += getConflicts(board_queens, row, board_queens[row]);
    }
    return total / 2; // 每对冲突都被计算了两次，所以除以2
}

// 初始化种群
void init()
{
    for (int i = 0; i < N; i++)
    {
        std::vector<int> b(SIZE);
        for (int j = 0; j < SIZE; j++)
        {
            b[j] = rand() % SIZE;
        }
        int conflicts = totalConflicts(b);
        boardQueue.push({b, conflicts});
    }
}

// 交叉操作：从两个父代创建一个子代
Board crossover(const Board &parent1, const Board &parent2)
{
    int crossover_point = rand() % SIZE;
    std::vector<int> child_queens(SIZE);
    for (int i = 0; i < crossover_point; ++i)
    {
        child_queens[i] = parent1.queens[i];
    }
    for (int i = crossover_point; i < SIZE; ++i)
    {
        child_queens[i] = parent2.queens[i];
    }
    int conflicts = totalConflicts(child_queens);
    return Board(child_queens, conflicts);
}

// 变异操作
void mutation(Board &board)
{
    if ((double)rand() / RAND_MAX < ABERRANCERATE)
    {
        int row = rand() % SIZE;
        int col = rand() % SIZE;
        board.queens[row] = col;
        board.score = totalConflicts(board.queens);
    }
}

// 检查是否达到目标（找到解决方案）
bool isGoal(const Board &board)
{
    return board.score == 0;
}

// 打印棋盘
void print(const Board &board)
{
    std::cout << "Solution found with " << board.score << " conflicts:" << std::endl;
    for (int i = 0; i < SIZE; ++i)
    {
        for (int j = 0; j < SIZE; ++j)
        {
            if (board.queens[i] == j)
            {
                std::cout << "Q ";
            }
            else
            {
                std::cout << ". ";
            }
        }
        std::cout << std::endl;
    }
}

int main()
{
    srand(time(0)); // 初始化随机数种子

    init(); // 初始化种群

    int generations = 0;
    const int max_generations = 10000;

    while (generations < max_generations)
    {
        if (isGoal(boardQueue.top()))
        {
            print(boardQueue.top());
            return 0;
        }

        // 从当前种群中选择最好的两个个体作为父代
        Board parent1 = boardQueue.top();
        boardQueue.pop();
        Board parent2 = boardQueue.top();
        boardQueue.pop();

        // 创建子代
        Board child = crossover(parent1, parent2);

        // 对子代进行变异
        mutation(child);

        // 将父代和子代放回种群
        boardQueue.push(parent1);
        boardQueue.push(parent2);
        boardQueue.push(child);

        // 保持种群大小恒定，移除最差的个体
        while (boardQueue.size() > N)
        {
            // 由于使用了std::greater，队列的"底部"（内部vector的末尾）实际上是分数最高的（最差的）
            // 但priority_queue不直接支持移除末尾元素，所以我们创建一个新的队列
            std::vector<Board> temp;
            while (!boardQueue.empty())
            {
                temp.push_back(boardQueue.top());
                boardQueue.pop();
            }
            // 移除最差的（最后一个）
            temp.pop_back();
            // 重建优先队列
            for (const auto &b : temp)
            {
                boardQueue.push(b);
            }
        }

        generations++;
        if (generations % 100 == 0)
        {
            std::cout << "Generation " << generations << ", Best score (conflicts): " << boardQueue.top().score << std::endl;
        }
    }

    std::cout << "No solution found within " << max_generations << " generations." << std::endl;
    std::cout << "Best board found:" << std::endl;
    print(boardQueue.top());

    return 0;
}