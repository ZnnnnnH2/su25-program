#include <SFML/Graphics.hpp>
#include <iostream>
#include <vector>
#include <array>
#include <limits>
#include <string>

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
};

std::vector<GameState> state_space;
char AI_PLAYER = 'O';

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

int evaluate_board(const Board &board)
{
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
    return full ? 2 : 0;
}

int is_terminal(int index) { return evaluate_board(state_space[index].board); }

int utility(int index)
{
    int t = is_terminal(index);
    if (AI_PLAYER == 'X')
    {
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
    else
    {
        switch (t)
        {
        case 1:
            return 0;
        case -1:
            return 10;
        case 2:
            return 5;
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
                int child_idx = (int)state_space.size() - 1;
                auto [_, cv] = min_node(child_idx);
                if (cv > best_val)
                {
                    best_val = cv;
                    best_child = child_idx;
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
                int child_idx = (int)state_space.size() - 1;
                auto [_, cv] = max_node(child_idx);
                if (cv < best_val)
                {
                    best_val = cv;
                    best_child = child_idx;
                }
            }
    state_space[index].minimax = best_val;
    state_space[index].best_move_index = best_child;
    return {best_child, best_val};
}

class TicTacToeGUI
{
private:
    sf::RenderWindow window;
    sf::Font font;
    GameState current;
    char human_player, ai_player;
    bool game_started;
    bool is_human_turn;
    sf::Clock ai_timer;
    bool ai_thinking;

    const int WINDOW_SIZE = 600;
    const int GRID_SIZE = 400;
    const int GRID_OFFSET = 100;
    const int CELL_SIZE = GRID_SIZE / 3;

public:
    TicTacToeGUI() : window(sf::VideoMode(WINDOW_SIZE, WINDOW_SIZE), "井字棋 AI 对战"),
                     game_started(false), is_human_turn(true), ai_thinking(false)
    {
        window.setFramerateLimit(60);

        // 修复：扩展字体路径，增加Linux常见路径
        std::vector<std::string> font_paths = {
            "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf",
            "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
            "/usr/share/fonts/TTF/DejaVuSans-Bold.ttf",
            "/usr/share/fonts/truetype/liberation/LiberationSans-Bold.ttf",
            "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
            "/usr/share/fonts/truetype/noto/NotoSans-Bold.ttf",
            "/usr/share/fonts/truetype/ubuntu/Ubuntu-B.ttf",
            "/System/Library/Fonts/Arial.ttf",
            "C:/Windows/Fonts/arial.ttf"};

        bool font_loaded = false;
        for (const auto &path : font_paths)
        {
            if (font.loadFromFile(path))
            {
                font_loaded = true;
                std::cout << "成功加载字体: " << path << "\n";
                break;
            }
        }

        if (!font_loaded)
        {
            std::cout << "警告：无法加载字体，将使用系统默认字体\n";
            // 即使没有加载字体，SFML也会使用默认字体，程序仍能运行
        }
    }

    void run()
    {
        showStartScreen();
        while (window.isOpen())
        {
            handleEvents();
            update();
            render();
        }
    }

private:
    void showStartScreen()
    {
        std::cout << "=== 井字棋 AI 对战 ===\n";
        std::cout << "请选择你的棋子:\n";
        std::cout << "按 X 键选择 X (先手)\n";
        std::cout << "按 O 键选择 O (后手)\n";
        std::cout << "或者点击窗口中的按钮\n";
    }

    void startGame(char player)
    {
        human_player = player;
        ai_player = (player == 'X') ? 'O' : 'X';
        AI_PLAYER = ai_player;
        game_started = true;
        is_human_turn = (human_player == 'X');

        if (!is_human_turn)
        {
            ai_thinking = true;
            ai_timer.restart();
        }

        std::cout << "游戏开始！你是 " << human_player << "，AI 是 " << ai_player << "\n";
        if (human_player == 'X')
        {
            std::cout << "你先手，点击棋盘下棋\n";
        }
        else
        {
            std::cout << "AI 先手，请等待...\n";
        }
    }

    void handleEvents()
    {
        sf::Event event;
        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
            {
                window.close();
            }

            if (!game_started)
            {
                if (event.type == sf::Event::KeyPressed)
                {
                    if (event.key.code == sf::Keyboard::X)
                    {
                        startGame('X');
                    }
                    else if (event.key.code == sf::Keyboard::O)
                    {
                        startGame('O');
                    }
                }
                // 修复：添加鼠标点击选择玩家
                if (event.type == sf::Event::MouseButtonPressed)
                {
                    if (event.mouseButton.button == sf::Mouse::Left)
                    {
                        int x = event.mouseButton.x;
                        int y = event.mouseButton.y;
                        // 检查是否点击了选择按钮区域
                        if (y >= 280 && y <= 320)
                        {
                            if (x >= 150 && x <= 250)
                            {
                                startGame('X'); // 点击 X 按钮
                            }
                            else if (x >= 350 && x <= 450)
                            {
                                startGame('O'); // 点击 O 按钮
                            }
                        }
                    }
                }
            }
            else
            {
                // 重新开始游戏
                if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::R)
                {
                    resetGame();
                }

                // 修复：只在合适的时机处理鼠标点击
                if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left)
                {
                    int game_status = evaluate_board(current.board);
                    if (game_status != 0)
                    {
                        // 游戏结束，点击重新开始
                        resetGame();
                    }
                    else if (is_human_turn && !ai_thinking)
                    {
                        // 人类回合且AI不在思考
                        handleMouseClick(event.mouseButton.x, event.mouseButton.y);
                    }
                }
            }
        }
    }

    void handleMouseClick(int x, int y)
    {
        if (x < GRID_OFFSET || x >= GRID_OFFSET + GRID_SIZE ||
            y < GRID_OFFSET || y >= GRID_OFFSET + GRID_SIZE)
        {
            return;
        }

        int col = (x - GRID_OFFSET) / CELL_SIZE;
        int row = (y - GRID_OFFSET) / CELL_SIZE;

        if (row >= 0 && row < 3 && col >= 0 && col < 3 &&
            current.board[row][col] == ' ')
        {

            current.board[row][col] = human_player;
            current.last_move = {row, col};

            std::cout << "你落子在 (" << row << "," << col << ")\n";

            // 检查游戏是否结束
            int game_status = evaluate_board(current.board);
            if (game_status == 0)
            {
                is_human_turn = false;
                ai_thinking = true;
                ai_timer.restart();
            }
        }
    }

    void update()
    {
        if (!game_started || is_human_turn || evaluate_board(current.board) != 0)
            return;

        // AI 思考延迟
        if (ai_thinking && ai_timer.getElapsedTime().asSeconds() < 1.0f)
        {
            return;
        }

        if (ai_thinking)
        {
            ai_thinking = false;
            makeAIMove();
        }
    }

    void makeAIMove()
    {
        GameState root = current;
        minimax(root);

        if (!state_space.empty() && state_space[0].best_move_index != -1)
        {
            int best_idx = state_space[0].best_move_index;
            Move m = state_space[best_idx].last_move;
            current.board[m.row][m.col] = ai_player;
            current.last_move = m;

            std::cout << "AI 落子在 (" << m.row << "," << m.col << ") [评分: "
                      << state_space[0].minimax << "]\n";
        }

        // 检查游戏是否结束
        int game_status = evaluate_board(current.board);
        if (game_status == 0)
        {
            is_human_turn = true;
        }
        else
        {
            // 游戏结束，输出结果
            if (game_status == 1)
            {
                std::cout << "🎉 X 获胜！\n";
            }
            else if (game_status == -1)
            {
                std::cout << "🎉 O 获胜！\n";
            }
            else
            {
                std::cout << "🤝 平局！\n";
            }
            std::cout << "按 R 键或点击屏幕重新开始\n";
        }
    }

    void resetGame()
    {
        current = GameState();
        is_human_turn = (human_player == 'X');
        ai_thinking = false;

        if (!is_human_turn)
        {
            ai_thinking = true;
            ai_timer.restart();
        }

        std::cout << "游戏重新开始！\n";
    }

    void render()
    {
        window.clear(sf::Color::White);

        if (!game_started)
        {
            renderStartScreen();
        }
        else
        {
            renderGame();
        }

        window.display();
    }

    void renderStartScreen()
    {
        // 标题
        sf::Text title("井字棋 AI 对战", font, 40);
        title.setFillColor(sf::Color::Black);
        title.setPosition(WINDOW_SIZE / 2 - title.getGlobalBounds().width / 2, 180);
        window.draw(title);

        // 选择提示
        sf::Text prompt("选择你的棋子:", font, 24);
        prompt.setFillColor(sf::Color::Black);
        prompt.setPosition(WINDOW_SIZE / 2 - prompt.getGlobalBounds().width / 2, 240);
        window.draw(prompt);

        // X 按钮
        sf::RectangleShape xButton(sf::Vector2f(80, 40));
        xButton.setPosition(150, 280);
        xButton.setFillColor(sf::Color(200, 200, 200));
        xButton.setOutlineColor(sf::Color::Black);
        xButton.setOutlineThickness(2);
        window.draw(xButton);

        sf::Text xText("X (先手)", font, 18);
        xText.setFillColor(sf::Color::Black);
        xText.setPosition(160, 290);
        window.draw(xText);

        // O 按钮
        sf::RectangleShape oButton(sf::Vector2f(80, 40));
        oButton.setPosition(350, 280);
        oButton.setFillColor(sf::Color(200, 200, 200));
        oButton.setOutlineColor(sf::Color::Black);
        oButton.setOutlineThickness(2);
        window.draw(oButton);

        sf::Text oText("O (后手)", font, 18);
        oText.setFillColor(sf::Color::Black);
        oText.setPosition(360, 290);
        window.draw(oText);

        // 键盘提示
        sf::Text keyHint("或按键盘 X / O 键", font, 16);
        keyHint.setFillColor(sf::Color(128, 128, 128));
        keyHint.setPosition(WINDOW_SIZE / 2 - keyHint.getGlobalBounds().width / 2, 340);
        window.draw(keyHint);
    }

    void renderGame()
    {
        // 绘制棋盘网格
        for (int i = 1; i < 3; i++)
        {
            sf::RectangleShape vline(sf::Vector2f(3, GRID_SIZE));
            vline.setPosition(GRID_OFFSET + i * CELL_SIZE, GRID_OFFSET);
            vline.setFillColor(sf::Color::Black);
            window.draw(vline);

            sf::RectangleShape hline(sf::Vector2f(GRID_SIZE, 3));
            hline.setPosition(GRID_OFFSET, GRID_OFFSET + i * CELL_SIZE);
            hline.setFillColor(sf::Color::Black);
            window.draw(hline);
        }

        // 绘制棋盘边框
        sf::RectangleShape border;
        border.setSize(sf::Vector2f(GRID_SIZE, GRID_SIZE));
        border.setPosition(GRID_OFFSET, GRID_OFFSET);
        border.setFillColor(sf::Color::Transparent);
        border.setOutlineColor(sf::Color::Black);
        border.setOutlineThickness(3);
        window.draw(border);

        // 绘制棋子
        for (int i = 0; i < 3; i++)
        {
            for (int j = 0; j < 3; j++)
            {
                if (current.board[i][j] != ' ')
                {
                    sf::Text piece(std::string(1, current.board[i][j]), font, 80);
                    if (current.board[i][j] == 'X')
                    {
                        piece.setFillColor(sf::Color::Red);
                    }
                    else
                    {
                        piece.setFillColor(sf::Color::Blue);
                    }

                    float x = GRID_OFFSET + j * CELL_SIZE + CELL_SIZE / 2 - piece.getGlobalBounds().width / 2;
                    float y = GRID_OFFSET + i * CELL_SIZE + CELL_SIZE / 2 - piece.getGlobalBounds().height / 2;
                    piece.setPosition(x, y);
                    window.draw(piece);
                }
            }
        }

        renderGameStatus();
    }

    void renderGameStatus()
    {
        std::string status;
        int game_status = evaluate_board(current.board);

        if (game_status == 1)
        {
            status = "🎉 X 获胜！点击重新开始";
        }
        else if (game_status == -1)
        {
            status = "🎉 O 获胜！点击重新开始";
        }
        else if (game_status == 2)
        {
            status = "🤝 平局！点击重新开始";
        }
        else if (ai_thinking)
        {
            status = "🤖 AI 正在思考...";
        }
        else if (is_human_turn)
        {
            status = "轮到你 (" + std::string(1, human_player) + ") - 点击下棋";
        }
        else
        {
            status = "轮到 AI (" + std::string(1, ai_player) + ")";
        }

        sf::Text statusText(status, font, 20);
        statusText.setFillColor(sf::Color::Black);
        statusText.setPosition(WINDOW_SIZE / 2 - statusText.getGlobalBounds().width / 2, 50);
        window.draw(statusText);

        // 显示玩家信息
        std::string playerInfo = "你: " + std::string(1, human_player) + " (红色)  AI: " + std::string(1, ai_player) + " (蓝色)";
        sf::Text infoText(playerInfo, font, 16);
        infoText.setFillColor(sf::Color::Black);
        infoText.setPosition(WINDOW_SIZE / 2 - infoText.getGlobalBounds().width / 2, 520);
        window.draw(infoText);

        // 操作提示
        sf::Text hintText("按 R 键重新开始", font, 14);
        hintText.setFillColor(sf::Color(128, 128, 128));
        hintText.setPosition(WINDOW_SIZE / 2 - hintText.getGlobalBounds().width / 2, 550);
        window.draw(hintText);
    }
};

int main()
{
    try
    {
        TicTacToeGUI game;
        game.run();
    }
    catch (const std::exception &e)
    {
        std::cerr << "错误: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}