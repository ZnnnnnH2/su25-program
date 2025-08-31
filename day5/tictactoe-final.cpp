#include <iostream>
#include <vector>
#include <array>
#include <limits>

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
    GameState(const GameState &s) = default;

    void print() const
    {
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
        std::cout << "\n";
    }
};

std::vector<GameState> state_space;
char AI_PLAYER = 'X'; // 全局变量记录AI的棋子

// 推断下一步落子方
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

// 评估棋盘（独立于 state_space，供对局循环使用）
int evaluate_board(const Board &board)
{
    for (int i = 0; i < 3; i++)
        if (board[i][0] != ' ' &&
            board[i][0] == board[i][1] && board[i][1] == board[i][2])
            return (board[i][0] == 'X') ? 1 : -1;
    for (int j = 0; j < 3; j++)
        if (board[0][j] != ' ' &&
            board[0][j] == board[1][j] && board[1][j] == board[2][j])
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
    return full ? 2 : 0;
}

// 下列函数依赖 state_space
int is_terminal(int index)
{
    return evaluate_board(state_space[index].board);
}

// 修改：从AI的角度评估效用值
int utility(int index)
{
    int t = is_terminal(index);
    if (AI_PLAYER == 'X')
    {
        // AI是X，X获胜最好，O获胜最差
        switch (t)
        {
        case 1:
            return 10; // X获胜
        case -1:
            return 0; // O获胜
        case 2:
            return 5; // 平局
        default:
            return 5;
        }
    }
    else
    {
        // AI是O，O获胜最好，X获胜最差
        switch (t)
        {
        case 1:
            return 0; // X获胜
        case -1:
            return 10; // O获胜
        case 2:
            return 5; // 平局
        default:
            return 5;
        }
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
    GameState initial = state;
    initial.current_player = infer_next_player(initial.board);
    initial.father = -1;
    initial.depth = 0;
    initial.best_move_index = -1;
    initial.game_over = false;
    state_space.push_back(initial);

    if (is_terminal(0) != 0)
    {
        state_space[0].game_over = true;
        state_space[0].minimax = utility(0);
        return;
    }

    // 修改：AI总是最大化玩家，对手总是最小化玩家
    if (initial.current_player == AI_PLAYER)
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
    int term = is_terminal(index);
    if (term != 0)
    {
        int u = utility(index);
        state_space[index].minimax = u;
        state_space[index].game_over = true;
        return {index, u};
    }
    int depth = state_space[index].depth;
    GameState node = state_space[index];
    int best_child = -1;
    int best_val = std::numeric_limits<int>::min();

    // 当前玩家应该是AI（最大化玩家）
    char current_player = state_space[index].current_player;
    char next_player = (current_player == 'X') ? 'O' : 'X';

    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++)
            if (can_move(index, {i, j}))
            {
                GameState child = node;
                child.board[i][j] = current_player;
                child.last_move = {i, j};
                child.current_player = next_player;
                child.father = index;
                child.depth = depth + 1;
                state_space.push_back(child);
                auto [ci, cv] = min_node((int)state_space.size() - 1);
                if (cv > best_val)
                {
                    best_val = cv;
                    best_child = (int)state_space.size() - 1;
                }
            }
    state_space[index].minimax = best_val;
    state_space[index].best_move_index = best_child;
    return {best_child, best_val};
}

std::pair<int, int> min_node(int index)
{
    int term = is_terminal(index);
    if (term != 0)
    {
        int u = utility(index);
        state_space[index].minimax = u;
        state_space[index].game_over = true;
        return {index, u};
    }
    int depth = state_space[index].depth;
    GameState node = state_space[index];
    int best_child = -1;
    int best_val = std::numeric_limits<int>::max();

    // 当前玩家应该是对手（最小化玩家）
    char current_player = state_space[index].current_player;
    char next_player = (current_player == 'X') ? 'O' : 'X';

    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++)
            if (can_move(index, {i, j}))
            {
                GameState child = node;
                child.board[i][j] = current_player;
                child.last_move = {i, j};
                child.current_player = next_player;
                child.father = index;
                child.depth = depth + 1;
                state_space.push_back(child);
                auto [ci, cv] = max_node((int)state_space.size() - 1);
                if (cv < best_val)
                {
                    best_val = cv;
                    best_child = (int)state_space.size() - 1;
                }
            }
    state_space[index].minimax = best_val;
    state_space[index].best_move_index = best_child;
    return {best_child, best_val};
}

// 简单打印棋盘
void print_board(const Board &b)
{
    std::cout << "  0 1 2\n";
    for (int i = 0; i < 3; i++)
    {
        std::cout << i << " ";
        for (int j = 0; j < 3; j++)
        {
            std::cout << b[i][j];
            if (j < 2)
                std::cout << "|";
        }
        std::cout << "\n";
        if (i < 2)
            std::cout << "  -----\n";
    }
    std::cout << "\n";
}

int main()
{
    // freopen("CON.out", "w", stdout);
    // std::ios::sync_with_stdio(false);
    // std::cin.tie(nullptr);

    GameState current;
    char human, ai;
    std::cout << "=== 井字棋 AI 对战 ===\n";
    std::cout << "选择你的棋子 (X 先手 / O 后手). 输入 X 或 O: ";
    while (true)
    {
        std::cin >> human;
        if (human == 'X' || human == 'x')
        {
            human = 'X';
            ai = 'O';
            break;
        }
        if (human == 'O' || human == 'o')
        {
            human = 'O';
            ai = 'X';
            break;
        }
        std::cout << "无效输入，重新输入 X 或 O: ";
    }

    AI_PLAYER = ai; // 设置全局AI棋子标识
    std::cout << "你是 " << human << "，AI 是 " << ai << "\n";
    std::cout << (human == 'X' ? "你先手\n" : "AI 先手\n") << "\n";
    print_board(current.board);

    while (true)
    {
        char to_move = infer_next_player(current.board);
        int status = evaluate_board(current.board);
        if (status != 0)
        {
            if (status == 1)
                std::cout << "🎉 X 获胜！\n";
            else if (status == -1)
                std::cout << "🎉 O 获胜！\n";
            else
                std::cout << "🤝 平局！\n";
            break;
        }

        if (to_move == human)
        {
            std::cout << "轮到你下 (" << human << ")，输入 行 列 (0-2): ";
            int r, c;
            while (true)
            {
                if (!(std::cin >> r >> c))
                {
                    std::cin.clear();
                    std::cin.ignore(1024, '\n');
                    std::cout << "输入格式错误，请重新输入 行 列: ";
                    continue;
                }
                if (r >= 0 && r < 3 && c >= 0 && c < 3 && current.board[r][c] == ' ')
                    break;
                std::cout << "位置无效或已被占用，请重新输入: ";
            }
            current.board[r][c] = human;
            current.last_move = {r, c};
            std::cout << "你落子在 (" << r << "," << c << ")\n";
            print_board(current.board);
        }
        else
        {
            std::cout << "🤖 AI 正在思考...\n";
            GameState root = current;
            minimax(root);
            if (state_space.empty() || state_space[0].best_move_index == -1)
            {
                std::cout << "AI 无法找到有效移动，游戏结束\n";
                break;
            }
            int best_idx = state_space[0].best_move_index;
            Move m = state_space[best_idx].last_move;
            current.board[m.row][m.col] = ai;
            current.last_move = m;
            std::cout << "AI 落子在 (" << m.row << "," << m.col << ") [评分: "
                      << state_space[0].minimax << "]\n";
            print_board(current.board);
        }
    }

    std::cout << "游戏结束！感谢游玩。\n";
    return 0;
}