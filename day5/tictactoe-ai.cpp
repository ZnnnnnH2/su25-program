// filepath: /home/zhang/class/su25-program/day5/tictactoe-final.cpp
#include <iostream>
#include <vector>
#include <array>

typedef std::array<std::array<char, 3>, 3> Board;

struct Move
{
    int row;
    int col;
};

struct GameState
{
    Board board;
    Move last_move;
    char current_player;
    bool game_over;
    int minimax;
    int best_move_index;
    int depth;
    int father;

    GameState()
    {
        for (int i = 0; i < 3; i++)
            for (int j = 0; j < 3; j++)
                board[i][j] = ' ';
        last_move = {-1, -1};
        current_player = 'X';
        game_over = false;
        minimax = 0;
        best_move_index = -1;
        depth = 0;
        father = -1;
    }

    // 修复：参数改为 const 引用，允许用临时 / const 对象拷贝
    GameState(const GameState &state)
    {
        board = state.board;
        last_move = state.last_move;
        current_player = state.current_player;
        game_over = state.game_over;
        minimax = state.minimax;
        best_move_index = state.best_move_index;
        depth = state.depth;
        father = state.father;
    }

    void print() const
    {
        std::cout << "棋盘状态 (深度: " << depth << ", 玩家: " << current_player << "):\n";
        std::cout << "  0 1 2\n";
        for (int i = 0; i < 3; i++)
        {
            std::cout << i << " ";
            for (int j = 0; j < 3; j++)
            {
                std::cout << board[i][j];
                if (j < 2)
                    std::cout << "|";
            }
            std::cout << "\n";
            if (i < 2)
                std::cout << "  -----\n";
        }
        std::cout << "Minimax值: " << minimax << "\n\n";
    }
};

std::vector<GameState> state_space;

// 推断下一步落子方（X 先手；若 X 与 O 数量相等则轮到 X，否则 O）
char infer_next_player(const Board &b)
{
    int cx = 0, co = 0;
    for (auto &r : b)
        for (char c : r)
        {
            if (c == 'X')
                cx++;
            else if (c == 'O')
                co++;
        }
    return (cx == co) ? 'X' : 'O';
}

int is_terminal(int index)
{
    const Board &board = state_space[index].board;
    for (int i = 0; i < 3; i++)
        if (board[i][0] != ' ' && board[i][0] == board[i][1] && board[i][1] == board[i][2])
            return (board[i][0] == 'X') ? 1 : -1;
    for (int j = 0; j < 3; j++)
        if (board[0][j] != ' ' && board[0][j] == board[1][j] && board[1][j] == board[2][j])
            return (board[0][j] == 'X') ? 1 : -1;
    if (board[0][0] != ' ' && board[0][0] == board[1][1] && board[1][1] == board[2][2])
        return (board[0][0] == 'X') ? 1 : -1;
    if (board[0][2] != ' ' && board[0][2] == board[1][1] && board[1][1] == board[2][0])
        return (board[0][2] == 'X') ? 1 : -1;

    bool full = true;
    for (int i = 0; i < 3 && full; i++)
        for (int j = 0; j < 3; j++)
            if (board[i][j] == ' ')
            {
                full = false;
                break;
            }
    if (full)
        return 2;
    return 0;
}

int utility(int index)
{
    int t = is_terminal(index);
    switch (t)
    {
    case 1:
        return 10;
    case -1:
        return 0;
    case 2:
        return 5;
    default:
        return 5;
    }
}

bool can_move(int index, Move move)
{
    if (move.row < 0 || move.row >= 3 || move.col < 0 || move.col >= 3)
        return false;
    return state_space[index].board[move.row][move.col] == ' ';
}

std::pair<int, int> max_node(int index);
std::pair<int, int> min_node(int index);

void minimax(const GameState &state)
{
    state_space.clear();
    GameState initial_state = state; // 去掉 const_cast
    initial_state.father = -1;
    initial_state.depth = 0;
    initial_state.current_player = infer_next_player(initial_state.board);
    initial_state.game_over = false;
    initial_state.minimax = 0;
    initial_state.best_move_index = -1;
    state_space.push_back(initial_state);

    if (is_terminal(0) != 0)
    {
        state_space[0].game_over = true;
        state_space[0].minimax = utility(0);
        return;
    }

    if (initial_state.current_player == 'X')
    {
        auto [best_idx, best_val] = max_node(0);
        state_space[0].best_move_index = best_idx;
        state_space[0].minimax = best_val;
    }
    else
    {
        auto [best_idx, best_val] = min_node(0);
        state_space[0].best_move_index = best_idx;
        state_space[0].minimax = best_val;
    }
}

std::pair<int, int> max_node(int index)
{
    if (state_space[index].current_player != 'X')
    {
        // 可添加断言
    }
    int term = is_terminal(index);
    if (term != 0)
    {
        int u = utility(index);
        state_space[index].minimax = u;
        state_space[index].game_over = true;
        return {index, u};
    }

    int depth = state_space[index].depth;
    GameState node_state = state_space[index];
    int best_child = -1;
    int best_val = -1000;

    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++)
            if (can_move(index, {i, j}))
            {
                GameState child = node_state;
                child.board[i][j] = 'X';
                child.last_move = {i, j};
                child.current_player = 'O';
                child.father = index;
                child.depth = depth + 1;
                state_space.push_back(child);
                auto [cidx, cval] = min_node((int)state_space.size() - 1);
                if (cval > best_val)
                {
                    best_val = cval;
                    best_child = (int)state_space.size() - 1;
                }
            }

    state_space[index].minimax = best_val;
    state_space[index].best_move_index = best_child;
    return {best_child, best_val};
}

std::pair<int, int> min_node(int index)
{
    if (state_space[index].current_player != 'O')
    {
        // 可添加断言
    }
    int term = is_terminal(index);
    if (term != 0)
    {
        int u = utility(index);
        state_space[index].minimax = u;
        state_space[index].game_over = true;
        return {index, u};
    }

    int depth = state_space[index].depth;
    GameState node_state = state_space[index];
    int best_child = -1;
    int best_val = 1000;

    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++)
            if (can_move(index, {i, j}))
            {
                GameState child = node_state;
                child.board[i][j] = 'O';
                child.last_move = {i, j};
                child.current_player = 'X';
                child.father = index;
                child.depth = depth + 1;
                state_space.push_back(child);
                auto [cidx, cval] = max_node((int)state_space.size() - 1);
                if (cval < best_val)
                {
                    best_val = cval;
                    best_child = (int)state_space.size() - 1;
                }
            }

    state_space[index].minimax = best_val;
    state_space[index].best_move_index = best_child;
    return {best_child, best_val};
}

void print_game_tree()
{
    std::cout << "=== 游戏树结构 ===\n\n";
    for (int i = 0; i < (int)state_space.size(); i++)
    {
        for (int d = 0; d < state_space[i].depth; d++)
            std::cout << "  ";
        std::cout << "节点 " << i
                  << " (深度 " << state_space[i].depth
                  << ", 父: " << state_space[i].father
                  << ", 轮到: " << state_space[i].current_player
                  << ", Minimax: " << state_space[i].minimax;
        if (state_space[i].last_move.row != -1)
            std::cout << ", 上一步: (" << state_space[i].last_move.row
                      << "," << state_space[i].last_move.col << ")";
        if (state_space[i].game_over)
            std::cout << " [终局]";
        std::cout << ")\n";
    }
    std::cout << "\n=== 最佳路径 ===\n";
    if (!state_space.empty())
    {
        int cur = 0;
        while (cur != -1 && state_space[cur].best_move_index != -1)
        {
            std::cout << "从节点 " << cur << " -> "
                      << state_space[cur].best_move_index << "\n";
            cur = state_space[cur].best_move_index;
        }
    }
}

int main()
{
    freopen("tictactoe-final.txt", "w", stdout);
    GameState initial;
    std::cout << "=== 井字棋 Minimax AI ===\n\n";
    initial.print();
    minimax(initial);
    std::cout << "根节点Minimax值: " << state_space[0].minimax << "\n";
    if (state_space[0].best_move_index != -1)
    {
        Move m = state_space[state_space[0].best_move_index].last_move;
        std::cout << "最佳第一步: (" << m.row << "," << m.col << ")\n";
    }
    print_game_tree();
    return 0;
}