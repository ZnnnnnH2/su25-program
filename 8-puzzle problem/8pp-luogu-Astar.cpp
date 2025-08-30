#include <algorithm>
#include <iostream>
#include <vector>
#include <queue>
#include <array>
#include <unordered_set>
#include <unordered_map>
#include <limits>
#include <stdexcept>

#define SIZE 3
#define SPACE '0'
#define MAX_DEPTH 32
#define INF 0x3f3f3f3f

typedef std::array<std::array<char, SIZE>, SIZE> Board;
const int positions[8][2] = {{0, 0}, {0, 1}, {0, 2}, {1, 2}, {2, 2}, {2, 1}, {2, 0}, {1, 0}};
const char expected[8] = {'1', '2', '3', '4', '5', '6', '7', '8'};

struct PuzzleBoard
{
    Board board{};
    int size;
    int blank_row;
    int blank_col;
    int father;
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

    bool operator<(const PuzzleBoard &other) const
    {
        return board < other.board;
    }
};

std::unordered_map<std::string, int> visited; // Store best cost to reach each state

bool (PuzzleBoard::*moveFunction[4])() = {
    &PuzzleBoard::moveUP,
    &PuzzleBoard::moveDOWN,
    &PuzzleBoard::moveLEFT,
    &PuzzleBoard::moveRIGHT};
int h = 0, t = 0;

struct AStarNode
{
    int f_cost; // f(n) = g(n) + h(n)
    int g_cost; // actual cost from start
    PuzzleBoard board;

    bool operator>(const AStarNode &other) const
    {
        return f_cost > other.f_cost;
    }
};

std::priority_queue<AStarNode, std::vector<AStarNode>, std::greater<AStarNode>> pq;

// int estimateCost(const Board &board)
// {
//     // Implement your heuristic function here
//     int sum = 0;
//     for (int i = 0; i < 8; i++)
//     {
//         int r = positions[i][0];
//         int c = positions[i][1];
//         if (board[r][c] != expected[i])
//         {
//             sum++;
//         }
//     }
//     return sum;
// }

int estimateCost(const Board &bodrd) // Manhattan distance
{
    int manhattan_distance = 0;

    // Calculate Manhattan distance for each number (1-8)
    for (int i = 0; i < SIZE; i++)
    {
        for (int j = 0; j < SIZE; j++)
        {
            char current = bodrd[i][j];
            if (current != SPACE && current >= '1' && current <= '8')
            {
                // Find the target position for this number
                int target_index = current - '1'; // Convert '1'-'8' to 0-7
                int target_row = positions[target_index][0];
                int target_col = positions[target_index][1];

                // Calculate Manhattan distance
                manhattan_distance += abs(i - target_row) + abs(j - target_col);
            }
        }
    }

    return manhattan_distance;
}
int ans = std::numeric_limits<int>::max();

void aStar(PuzzleBoard &initialBoard)
{
    AStarNode startNode = {estimateCost(initialBoard.board), 0, initialBoard};
    pq.push(startNode);
    visited[initialBoard.getBoardString()] = 0;

    while (!pq.empty())
    {
        AStarNode current = pq.top();
        pq.pop();

        // Prune using the A* lower bound (f = g + h). Stronger than only g.
        if (current.f_cost >= ans)
            continue;

        if (current.board.isGoal())
        {
            ans = std::min(ans, current.g_cost);
            continue;
        }

        // Skip if we've found a better path to this state
        auto it = visited.find(current.board.getBoardString());
        if (it != visited.end() && it->second < current.g_cost)
            continue;

        for (int i = 0; i < 4; i++)
        {
            PuzzleBoard child = current.board;
            if ((child.*moveFunction[i])())
            {
                int newGCost = current.g_cost + 1;
                int newHCost = estimateCost(child.board);
                int newFCost = newGCost + newHCost;

                std::string childStr = child.getBoardString();
                auto childIt = visited.find(childStr);

                // Only add if we haven't seen this state or found a better path
                if (childIt == visited.end() || childIt->second > newGCost)
                {
                    AStarNode childNode = {newFCost, newGCost, child};
                    pq.push(childNode);
                    visited[childStr] = newGCost;
                }
            }
        }
    }
}

int main()
{
    // freopen("1.in", "r", stdin);
    // freopen("1.out", "w", stdout);
    Board board;

    // Read input as a single number
    std::string input;
    std::cin >> input;

    // Convert string to 3x3 board
    int idx = 0;
    for (int i = 0; i < SIZE; i++)
    {
        for (int j = 0; j < SIZE; j++)
        {
            board[i][j] = input[idx++];
        }
    }

    PuzzleBoard initPuzzleBoard(board);
    aStar(initPuzzleBoard);
    printf("%d\n", ans);
}