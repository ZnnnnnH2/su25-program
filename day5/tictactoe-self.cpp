#include <iostream>
#include <vector>
#include <array>

typedef std::array<std::array<char, 3>, 3> Board;

struct Move{
    int row;
    int col;
};
//let's say X (maxplayer) first and O second
struct GameState{
    Board board;
    Move last_move;  //last move made by the current player(how to get to the board)
    char current_player; //X or O
    bool game_over; //chack leaf node
    int minimax; //value for X player, if game_over is true then it means Utility value
    int best_move_index; //index of the best move in the state_space vector
    int depth;
    int father; //index of the father in the state_space vector

    // init
    GameState();
    GameState(GameState& state);

    //print
    void print() const;
};

std::vector<GameState> state_space;


// o for draw, 1 for win for maxplayer, -1 for win for minplayer
int is_terminal(int index){
    
}

// for leaf node, 0~10; 5 for draw, 10 for best result for maxplayer, 0 for best result for minplayer
int utility(int index){
    
}

bool can_move(int index, Move move){
    
}

void minimax(GameState& state){
    GameState empty_state;
    empty_state.father = -1;
    empty_state.depth = 0;
    empty_state.current_player = 'O';
    empty_state.game_over = false;
    empty_state.minimax = 0;
    state_space.push_back(empty_state);
    auto [best_move_index, best_move_value] = max_node(0);
    empty_state.best_move_index = best_move_index;
    empty_state.minimax = best_move_value;
}
std::pair<int, int> max_node(int index){
    if (is_terminal(index)){
        return {index, utility(index)};
    }
    int depth = state_space[index].depth;
    GameState state = state_space[index];
    char current_player = state.current_player == 'X' ? 'O' : 'X';
    int best_move_index = -1;
    int best_move_value = -1000;
    for(int i = 0; i < 3; i++){
        for(int j = 0; j < 3; j++){
            if(can_move(index, {i, j})){
                Move move = {i, j};
                GameState new_state = state;
                new_state.board[i][j] = current_player;
                new_state.last_move = move;
                new_state.current_player = current_player;
                new_state.father = index;
                new_state.depth = depth + 1;
                state_space.push_back(new_state);
                auto [move_index, move_value] = min_node(state_space.size() - 1);
                if (best_move_value < move_value){
                    best_move_value = move_value;
                    best_move_index = move_index;
                }
            }
        }
    }
    return {best_move_index, best_move_value};
}
std::pair<int, int> min_node(int index){
    if (is_terminal(index)){
        return {index, utility(index)};
    }
    int depth = state_space[index].depth;
    GameState state = state_space[index];
    char current_player = state.current_player == 'X' ? 'O' : 'X';
    int best_move_index = -1;
    int best_move_value = 1000;
    for(int i = 0; i < 3; i++){
        for(int j = 0; j < 3; j++){
            if(can_move(index, {i, j})){
                Move move = {i, j};
                GameState new_state = state;
                new_state.board[i][j] = current_player;
                new_state.last_move = move;
                new_state.current_player = current_player;
                new_state.father = index;
                new_state.depth = depth + 1;
                state_space.push_back(new_state);
                auto [move_index, move_value] = max_node(state_space.size() - 1);
                if (best_move_value > move_value){
                    best_move_value = move_value;
                    best_move_index = move_index;
                }
            }
        }
    }
    return {best_move_index, best_move_value};
}


// print the tree structure and the minimax value for each node
void print_game_tree(){
    
}

int main()
{

    return 0;
}