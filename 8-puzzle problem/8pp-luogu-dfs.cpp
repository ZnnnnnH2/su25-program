#include <algorithm>
#include <iostream>
#include <vector>
#include <queue>
#include <array>
#include <unordered_set>
#include <stack>

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
};

std::unordered_set<std::string> visited;

bool (PuzzleBoard::*moveFunction[4])() = {
    &PuzzleBoard::moveUP,
    &PuzzleBoard::moveDOWN,
    &PuzzleBoard::moveLEFT,
    &PuzzleBoard::moveRIGHT};

std::vector<PuzzleBoard> chessVector;
int h = 0, t = 0;

int printPath(int index)
{
    if (index == -1)
    {
        return 1;
    }
    int step = printPath(chessVector[index].getFather());
    // printf("step %d:\n", step);
    // chessVector[index].printBoard();
    // printf("\n");
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
            int ans = printPath(h);
            printf("%d\n", ans - 2);
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

// Structure to store DFS state information
struct DFSState
{
    PuzzleBoard board;
    int depth;
    std::vector<PuzzleBoard> path;

    DFSState(PuzzleBoard b, int d, std::vector<PuzzleBoard> p)
        : board(b), depth(d), path(p) {}
};

void dfs(PuzzleBoard initial)
{
    std::stack<DFSState> dfsStack;
    std::vector<PuzzleBoard> initialPath;
    initialPath.push_back(initial);
    dfsStack.push(DFSState(initial, 0, initialPath));

    while (!dfsStack.empty())
    {
        DFSState current = dfsStack.top();
        dfsStack.pop();

        if (current.depth > MAX_DEPTH)
        {
            continue;
        }
        if (current.depth > dfsDepth)
        {
            continue;
        }

        if (current.board.isGoal())
        {
            if (current.path.size() < dfsDepth)
            {
                dfsDepth = current.depth;
                dfsAnswer = current.path;
            }
            continue;
        }

        for (int i = 0; i < 4; i++)
        {
            PuzzleBoard child = current.board;
            if ((child.*moveFunction[i])())
            {
                std::string boardString = child.getBoardString();
                if (visited.find(boardString) != visited.end())
                {
                    continue;
                }
                visited.insert(boardString);

                std::vector<PuzzleBoard> newPath = current.path;
                newPath.push_back(child);
                dfsStack.push(DFSState(child, current.depth + 1, newPath));

                visited.erase(boardString);
            }
        }
    }
}

int main()
{
    // freopen("1.in", "r", stdin);
    // freopen("1.out", "w", stdout);
    Board board;
    for (int i = 0; i < SIZE; i++)
    {
        for (int j = 0; j < SIZE; j++)
        {
            char c;
            scanf("%c", &c); // The space before %c skips whitespace
            board[i][j] = c;
        }
    }
    PuzzleBoard initPuzzleBoard(board);
    chessVector.push_back(initPuzzleBoard);
    t++;
    // bfs();
    visited.clear();
    dfs(initPuzzleBoard);
    printf("%d\n", dfsDepth);
}