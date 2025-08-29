#include <algorithm>
#include <iostream>
#include <vector>
#include <queue>
#include <array>
#include <unordered_set>

#define SIZE 3
#define SPACE '0'
#define MAX_DEPTH 32
#define INF 0x3f3f3f3f

typedef std::array<std::array<char, SIZE>, SIZE> Board;
const int positions[8][2] = {{0, 0}, {0, 1}, {0, 2}, {1, 2}, {2, 2}, {2, 1}, {2, 0}, {1, 0}};
const char expected[8] = {'1', '2', '3', '4', '5', '6', '7', '8'};

class PuzzleBoard
{
private:
    Board board{};
    int size;
    int blank_row;
    int blank_col;
    int father;

public:
    PuzzleBoard()
    {
        this->size = SIZE;
        this->father = -1;
    }
    PuzzleBoard(const Board &board)
    {
        this->board = board;
        this->size = SIZE;
        findBlank();
        this->father = -1;
    }

    void findBlank()
    {
        for (int i = 0; i < size; i++)
        {
            for (int j = 0; j < size; j++)
            {
                if (board[i][j] == SPACE)
                {
                    blank_row = i;
                    blank_col = j;
                    return; // Exit function when blank is found
                }
            }
        }
        throw std::runtime_error("Blank not found");
    }

    void printBoard()
    {
        for (int i = 0; i < size; i++)
        {
            for (int j = 0; j < size; j++)
            {
                printf("%c ", board[i][j]);
            }
            printf("\n");
        }
    }

    bool isGoal()
    {
        // Center must be the blank
        if (board[1][1] != SPACE)
        {
            return false;
        }
        // Clockwise loop around center: 1..8

        for (int k = 0; k < 8; ++k)
        {
            int r = positions[k][0];
            int c = positions[k][1];
            if (board[r][c] != expected[k])
            {
                return false;
            }
        }
        return true;
    }

    bool move(int directionX, int directionY)
    {
        if (blank_row + directionX < 0 || blank_row + directionX >= size || blank_col + directionY < 0 || blank_col + directionY >= size)
        {
            return false;
        }
        std::swap(board[blank_row][blank_col], board[blank_row + directionX][blank_col + directionY]);
        blank_row += directionX;
        blank_col += directionY;
        return true;
    }
    bool moveUP()
    {
        return move(-1, 0);
    }
    bool moveDOWN()
    {
        return move(1, 0);
    }
    bool moveLEFT()
    {
        return move(0, -1);
    }
    bool moveRIGHT()
    {
        return move(0, 1);
    }

    void setFather(int father)
    {
        this->father = father;
    }
    int getFather()
    {
        return father;
    }

    std::string getBoardString()
    {
        std::string boardString;
        for (int i = 0; i < size; i++)
        {
            for (int j = 0; j < size; j++)
            {
                boardString += board[i][j];
            }
        }
        return boardString;
    }
    int majorizedFunction()
    {
        int sum = 0;
        for (int i = 0; i < 8; i++)
        {
            int value = board[positions[i][0]][positions[i][1]] - '0'; // Convert char to int
            if (value > 0)                                             // Only calculate for non-zero values
            {
                // Calculate Manhattan distance from current position to goal position
                int goalRow = (value - 1) / 3;
                int goalCol = (value - 1) % 3;
                int currentRow = positions[i][0];
                int currentCol = positions[i][1];

                sum += abs(currentRow - goalRow) + abs(currentCol - goalCol);
            }
        }
        return sum;
    }
};

std::unordered_set<std::string> visited;

bool (PuzzleBoard::*moveFunction[4])() = {PuzzleBoard::moveUP, PuzzleBoard::moveDOWN, PuzzleBoard::moveLEFT, PuzzleBoard::moveRIGHT};

std::vector<PuzzleBoard> chessVector;
int h = 0, t = 0;

int printPath(int index)
{
    if (index == -1)
    {
        return 1;
    }
    int step = printPath(chessVector[index].getFather());
    printf("step %d:\n", step);
    chessVector[index].printBoard();
    printf("\n");
    return step + 1;
}

void bfs()
{
    while (h < t)
    {
        PuzzleBoard current = chessVector[h];
        std::string boardString = current.getBoardString();
        if (visited.find(boardString) != visited.end())
        {
            h++;
            continue;
        }
        visited.insert(boardString);
        if (current.isGoal())
        {
            printPath(h);
            return;
        }
        // move
        for (int i = 0; i < 4; i++)
        {
            PuzzleBoard child = current;
            if ((child.*moveFunction[i])())
            {
                child.setFather(h);
                chessVector.push_back(child);
                t++;
            }
        }
        h++;
    }
}

std::vector<PuzzleBoard> dfsAnswer;
int dfsDepth = INF;

void dfs(PuzzleBoard current, int depth, std::vector<PuzzleBoard> result)
{
    if (depth > MAX_DEPTH)
    {
        return;
    }
    if (depth > dfsDepth)
    {
        return;
    }
    std::string boardString = current.getBoardString();
    result.push_back(current);
    if (current.isGoal())
    {
        if (result.size() < dfsDepth)
        {
            dfsDepth = depth;
            dfsAnswer = result;
        }
        return;
    }
    for (int i = 0; i < 4; i++)
    {
        PuzzleBoard child = current;
        if ((child.*moveFunction[i])())
        {
            boardString = child.getBoardString();
            if (visited.find(boardString) != visited.end())
            {
                continue;
            }
            visited.insert(boardString);
            dfs(child, depth + 1, result);
            visited.erase(boardString);
        }
    }
    result.pop_back();
    return;
}

int main()
{
    freopen("1.in", "r", stdin);
    freopen("1.out", "w", stdout);
    Board board;
    for (int i = 0; i < SIZE; i++)
    {
        for (int j = 0; j < SIZE; j++)
        {
            char c;
            scanf(" %c", &c); // The space before %c skips whitespace
            board[i][j] = c;
        }
    }
    PuzzleBoard initPuzzleBoard(board);
    chessVector.push_back(initPuzzleBoard);
    t++;
    bfs();
    visited.clear();
    std::vector<PuzzleBoard> result;
    dfs(initPuzzleBoard, 0, result);
    for (int i = 0; i < dfsAnswer.size(); i++)
    {
        printf("step %d:\n", i + 1);
        dfsAnswer[i].printBoard();
        printf("\n");
    }
}