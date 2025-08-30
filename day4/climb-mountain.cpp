// climb mountain to solve  queen
#include <iostream>

#define SIZE 128

int board[SIZE]; // board[i] 表示第i行皇后所在的列

// 统计某一行某一列的冲突数
int getConflicts(int row, int col)
{
    int cnt = 0;
    for (int i = 0; i < SIZE; ++i)
    {
        if (i == row)
            continue;
        if (board[i] == col)
            cnt++; // 列冲突
        if (abs(i - row) == abs(board[i] - col))
            cnt++; // 对角线冲突
    }
    return cnt;
}

// 统计整个棋盘冲突数
int totalConflicts()
{
    int total = 0;
    for (int row = 0; row < SIZE; ++row)
    {
        total += getConflicts(row, board[row]);
    }
    return total / 2; // 每对冲突统计了两次
}

void randomGenerate()
{
    srand(time(0));
    for (int i = 0; i < SIZE; ++i)
    {
        board[i] = rand() % SIZE;
    }
}

void printBoard()
{
    for (int i = 0; i < SIZE; i++)
    {
        printf("%d ", board[i]);
    }
    printf("\n");
}

void climbMountain()
{
    int t = 0;
    int lastConf = totalConflicts();
    while (true)
    {
        printf("turn:%d totalConf:%d\n", t, lastConf);
        t++;
        printBoard();
        int bestRow = -1, bestCol = -1, minConf = lastConf;
        // 对每一行的皇后，尝试移动到其他列
        for (int row = 0; row < SIZE; ++row)
        {
            int curCol = board[row];
            for (int col = 0; col < SIZE; ++col)
            {
                if (col == curCol)
                    continue;
                int oldCol = board[row];
                board[row] = col;
                int conf = totalConflicts();
                if (conf < minConf)
                {
                    minConf = conf;
                    bestRow = row;
                    bestCol = col;
                }
                board[row] = oldCol;
            }
        }
        if (minConf < lastConf)
        {
            board[bestRow] = bestCol;
            lastConf = minConf;
        }
        else
        {
            break; // 无法继续优化
        }
        if (lastConf == 0)
            break; // 已无冲突
    }
}

int main()
{
    randomGenerate();
    std::cout << "初始棋盘：" << std::endl;
    printBoard();
    climbMountain();
    std::cout << "爬山算法结果：" << std::endl;
    printBoard();
    return 0;
}
